#include "firebase_manager.h"
#include "addons/TokenHelper.h"
#include <Arduino.h>

namespace FirebaseManager
{
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;

    // ── Connect & authenticate ────────────────────────────────────────────
    void connectFirebase()
    {
        config.api_key = FIREBASE_API_KEY;

        auth.user.email = FIREBASE_AUTH_EMAIL;
        auth.user.password = FIREBASE_AUTH_PASS;

        config.token_status_callback = tokenStatusCallback; // from TokenHelper.h

        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);

        Serial.print("\n[Firebase] Authenticating");
        while (!Firebase.ready())
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\n[Firebase] Authenticated & connected!\n");
    }

    // ── Write one document with both readings ─────────────────────────────
    bool sendFlowReadings(float flowIn, float flowOut)
    {
        if (!Firebase.ready())
        {
            Serial.println("[Firebase] Not ready – skipping write.");
            return false;
        }

        // Build document path:  flow_readings/esp32_node_01/readings/<timestamp_ms>
        String docPath;
        docPath = "flow_readings/";
        docPath += DEVICE_ID;
        docPath += "/readings/";
        docPath += String(millis());

        FirebaseJson doc;
        doc.set("fields/FlowSensor1_In/doubleValue", flowIn);
        doc.set("fields/FlowSensor2_Out/doubleValue", flowOut);
        doc.set("fields/timestamp_ms/integerValue", (int)millis());

        bool ok = Firebase.Firestore.createDocument(
            &fbdo,
            FIREBASE_PROJECT_ID,
            "", // database id (empty = default)
            docPath.c_str(),
            doc.raw());

        if (ok)
        {
            Serial.printf("[Firebase] ✓ Written → In: %.2f  Out: %.2f L/min\n",
                          flowIn, flowOut);
        }
        else
        {
            Serial.print("[Firebase] ✗ Error: ");
            Serial.println(fbdo.errorReason());
        }

        return ok;
    }
}