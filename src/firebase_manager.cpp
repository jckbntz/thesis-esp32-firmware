// ─────────────────────────────────────────────────────────────────────────────
//  firebase_manager.cpp
//
//  FirebaseClient v2.1+ API — three breaking changes from older examples:
//
//  FIXED 1 — DefaultNetwork / getNetwork() REMOVED in v2.1.0
//    OLD: DefaultNetwork network; AsyncClient aClient(ssl, getNetwork(network));
//    NEW: AsyncClient aClient(ssl_client);   ← ssl only, no network wrapper
//
//  FIXED 2 — Build flags must be defined BEFORE FirebaseClient.h is included.
//    Placing them only in platformio.ini build_flags is not enough when the
//    header is pulled in through a .cpp file.  They are now defined in
//    firebase_manager.h with #ifndef guards so they compile exactly once.
//
//  FIXED 3 — Values::ArrayValue has no default constructor.
//    OLD: Values::ArrayValue arr;  arr.add(...);   ← compile error
//    NEW: Build each sensor map fully first, then construct ArrayValue with
//         the first element and chain .add() for the rest.
// ─────────────────────────────────────────────────────────────────────────────

#include "firebase_manager.h"
#include <Arduino.h>

namespace FirebaseManager
{
    // ── v2.1+ objects ─────────────────────────────────────────────────────────
    //  UserAuth  — stores API key + email/password credentials
    //  FirebaseApp — manages the token lifecycle (fetch, cache, refresh)
    //  WiFiClientSecure — SSL/TLS transport (built into ESP32 Arduino core)
    //  AsyncClientClass — request queue; takes ssl_client directly (no network
    //                     wrapper since v2.1.0)
    static UserAuth             userAuth(FIREBASE_API_KEY,
                                         FIREBASE_EMAIL,
                                         FIREBASE_PASSWORD,
                                         3000 /* token TTL seconds, max 3600 */);
    static FirebaseApp          app;
    static WiFiClientSecure     sslClient;
    using  AsyncClient        = AsyncClientClass;
    static AsyncClient          aClient(sslClient);   // ← no getNetwork() here
    static Firestore::Documents Docs;
    static AsyncResult          aResult;

    // ── begin() ───────────────────────────────────────────────────────────────
    void begin()
    {
        // setInsecure() skips certificate verification.
        // Adequate for development; for production pin the Google root CA cert.
        sslClient.setInsecure();

        // Hand credentials to the library — it authenticates asynchronously
        // and will auto-refresh the token before it expires.
        initializeApp(aClient, app, getAuth(userAuth), aResult);

        // Bind the Firestore Documents service to the authenticated app.
        app.getApp<Firestore::Documents>(Docs);

        // Block until the first token is ready.
        Serial.print("\n[Firebase] Authenticating");
        while (!app.ready())
        {
            app.loop();
            delay(200);
            Serial.print(".");
        }
        Serial.println("\n[Firebase] Ready!\n");
    }

    // ── loop() ────────────────────────────────────────────────────────────────
    //  Must run every Arduino loop() — keeps the token alive and processes the
    //  async task queue.  Removing this call causes auth to expire in ~1 hour.
    void loop()
    {
        app.loop();
        Docs.loop();
    }

    // ── buildSensorMap() ──────────────────────────────────────────────────────
    //  Helper: wraps one FlowSensor slot into a Firestore MapValue.
    //  Kept separate so sendSensors() stays readable.
    static Values::MapValue buildSensorMap(int index, const FlowSensor& s)
    {
        Values::MapValue m("sensor_index",
            Values::Value(Values::IntegerValue(index)));

        m.add("gpio_pin",
            Values::Value(Values::IntegerValue(static_cast<int>(s.pin))));

        m.add("flow_lpm",
            Values::Value(Values::DoubleValue(s.currentFlow)));

        m.add("total_l",
            Values::Value(Values::DoubleValue(s.totalLitres)));

        return m;
    }

    // ── sendSensors() ─────────────────────────────────────────────────────────
    //
    //  Produces this Firestore document structure:
    //
    //  {
    //    "node_id":   "esp32_node_01",        ← StringValue
    //    "uptime_ms": 12000,                  ← IntegerValue
    //    "sensors": [                         ← ArrayValue of MapValues
    //      { "sensor_index":1, "gpio_pin":34,
    //        "flow_lpm":12.50, "total_l":105.20 },
    //      ...10 entries total
    //    ]
    //  }
    //
    //  Values::ArrayValue has no default constructor — it must receive its
    //  first element at construction time.  We build sensor[0] separately,
    //  construct the array with it, then loop from sensor[1] onwards.
    // ─────────────────────────────────────────────────────────────────────────
    bool sendSensors(const FlowSensor* sensors, int count)
    {
        if (!app.ready())
        {
            Serial.println("[Firebase] Not ready — skipping write.");
            return false;
        }

        if (count < 1)
        {
            Serial.println("[Firebase] No sensors — skipping write.");
            return false;
        }

        // ── 1. Build top-level document ───────────────────────────────────────
        Document<Values::Value> doc(
            "node_id", Values::Value(Values::StringValue(DEVICE_ID)));

        doc.add("uptime_ms",
            Values::Value(Values::IntegerValue(static_cast<int>(millis()))));

        // ── 2. Build the sensors ArrayValue ───────────────────────────────────
        //  Construct with sensor[0] to satisfy the no-default-constructor rule,
        //  then append the remaining sensors.
        Values::ArrayValue sensorArray(
            Values::Value(buildSensorMap(1, sensors[0])));

        for (int i = 1; i < count; i++)
        {
            sensorArray.add(
                Values::Value(buildSensorMap(i + 1, sensors[i])));
        }

        doc.add("sensors", Values::Value(sensorArray));

        // ── 3. Build document path ────────────────────────────────────────────
        //  flow_readings/<device_id>/readings/<uptime_ms>
        String docPath = "flow_readings/";
        docPath += DEVICE_ID;
        docPath += "/readings/";
        docPath += String(millis());

        // ── 4. Synchronous write ──────────────────────────────────────────────
        //  Passing aResult without a callback → sync/blocking mode.
        //  aClient.lastError().code() == 0 means success.
        Docs.createDocument(
            aClient,
            Firestore::Parent(FIREBASE_PROJECT_ID),
            docPath,
            DocumentMask(),
            doc);

        if (aClient.lastError().code() == 0)
        {
            Serial.printf("[Firebase] ✓ Written → %s\n", docPath.c_str());
            return true;
        }

        Serial.printf("[Firebase] ✗ Error %d — %s\n",
                      aClient.lastError().code(),
                      aClient.lastError().message().c_str());
        return false;
    }

} // namespace FirebaseManager