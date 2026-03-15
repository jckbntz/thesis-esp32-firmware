#include "firebase_manager.h"
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Payload shape written to Firestore (and mirrored to Serial):
//
//  {
//    "node_id":    "esp32_node_01",
//    "uptime_ms":  22071,
//    "sensors": [
//      { "sensor_index":1, "gpio_pin":34,
//        "flow_lpm":2.94,  "total_l":2.09 },
//      ...
//    ],
//    "segments": [
//      {
//        "segment_id":  "seg_A",
//        "inflow_idx":  0,
//        "outflow_idx": 1,
//        "inflow_lpm":  2.94,
//        "outflow_lpm": 2.81,
//        "diff_pct":    4.42,
//        "status":      "NORMAL",
//        "leaked_lpm":  0.0
//      },
//      ...
//    ],
//    "alerts": [
//      {
//        "segment_id": "seg_B",
//        "status":     "CONFIRMED",
//        "diff_pct":   18.5,
//        "leaked_lpm": 0.71
//      }
//    ]
//  }
//
//  "alerts" contains only SUSPECTED and CONFIRMED segments.
//  React reads alerts[] to fire push notifications — no physics needed there.
// ─────────────────────────────────────────────────────────────────────────────

namespace FirebaseManager
{
    static UserAuth             userAuth(FIREBASE_API_KEY,
                                         FIREBASE_EMAIL,
                                         FIREBASE_PASSWORD,
                                         3000);
    static FirebaseApp          app;
    static WiFiClientSecure     sslClient;
    using  AsyncClient        = AsyncClientClass;
    static AsyncClient          aClient(sslClient);
    static Firestore::Documents Docs;
    static AsyncResult          aResult;

    // ── Internal helpers ──────────────────────────────────────────────────────
    static const char* statusLabel(LeakStatus s)
    {
        switch (s)
        {
            case LeakStatus::SUSPECTED: return "SUSPECTED";
            case LeakStatus::CONFIRMED: return "CONFIRMED";
            default:                    return "NORMAL";
        }
    }

    // ── begin() ───────────────────────────────────────────────────────────────
    void begin()
    {
        sslClient.setInsecure();
        initializeApp(aClient, app, getAuth(userAuth), aResult);
        app.getApp<Firestore::Documents>(Docs);

        Serial.print("\n[Firebase] Authenticating");
        while (!app.ready())
        {
            app.loop();
            delay(200);
            Serial.print(".");
        }
        Serial.println("\n[Firebase] Ready!\n");
    }

    void loop()
    {
        app.loop();
        Docs.loop();
    }

    // ── printReading() ────────────────────────────────────────────────────────
    //  Prints the complete payload to Serial in valid JSON format.
    //  Mirrors exactly what would be written to Firestore.
    //  Zero cost — no quota consumed.
    void printReading(const FlowSensor*  sensors,  int sensorCount,
                      const PipeSegment* segments, int segmentCount)
    {
        const unsigned long now = millis();

        Serial.printf("\n[Serial] ── Reading ─────────────── uptime: %lu ms\n", now);
        Serial.println("{");
        Serial.printf("  \"node_id\": \"%s\",\n", DEVICE_ID);
        Serial.printf("  \"uptime_ms\": %lu,\n", now);

        // ── sensors array ─────────────────────────────────────────────────────
        Serial.println("  \"sensors\": [");
        for (int i = 0; i < sensorCount; i++)
        {
            const bool last = (i == sensorCount - 1);
            Serial.printf(
                "    {\"sensor_index\":%d,\"gpio_pin\":%d,"
                "\"flow_lpm\":%.2f,\"total_l\":%.2f}%s\n",
                i + 1,
                static_cast<int>(sensors[i].pin),
                sensors[i].currentFlow,
                sensors[i].totalLitres,
                last ? "" : ","
            );
        }
        Serial.println("  ],");

        // ── segments array ────────────────────────────────────────────────────
        Serial.println("  \"segments\": [");
        for (int i = 0; i < segmentCount; i++)
        {
            const PipeSegment& seg = segments[i];
            const LeakResult&  r   = seg.last_result;
            const bool         last = (i == segmentCount - 1);

            Serial.printf(
                "    {\"segment_id\":\"%s\","
                "\"inflow_idx\":%d,\"outflow_idx\":%d,"
                "\"inflow_lpm\":%.2f,\"outflow_lpm\":%.2f,"
                "\"diff_pct\":%.2f,\"status\":\"%s\","
                "\"leaked_lpm\":%.2f}%s\n",
                seg.segment_id,
                seg.inflow_idx, seg.outflow_idx,
                sensors[seg.inflow_idx ].currentFlow,
                sensors[seg.outflow_idx].currentFlow,
                r.diff_pct,
                statusLabel(r.status),
                r.leaked_lpm,
                last ? "" : ","
            );
        }
        Serial.println("  ],");

        // ── alerts array (SUSPECTED + CONFIRMED only) ─────────────────────────
        Serial.println("  \"alerts\": [");
        bool firstAlert = true;
        for (int i = 0; i < segmentCount; i++)
        {
            const PipeSegment& seg = segments[i];
            if (seg.last_result.status == LeakStatus::NORMAL) continue;

            if (!firstAlert) Serial.println(",");
            firstAlert = false;

            Serial.printf(
                "    {\"segment_id\":\"%s\","
                "\"status\":\"%s\","
                "\"diff_pct\":%.2f,"
                "\"leaked_lpm\":%.2f}",
                seg.segment_id,
                statusLabel(seg.last_result.status),
                seg.last_result.diff_pct,
                seg.last_result.leaked_lpm
            );
        }
        if (!firstAlert) Serial.println();
        Serial.println("  ]");
        Serial.println("}");
    }

    // ── Firestore helpers ─────────────────────────────────────────────────────
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

    static Values::MapValue buildSegmentMap(const PipeSegment& seg,
                                            const FlowSensor*  sensors)
    {
        const LeakResult& r = seg.last_result;

        Values::MapValue m("segment_id",
            Values::Value(Values::StringValue(seg.segment_id)));
        m.add("inflow_idx",
            Values::Value(Values::IntegerValue(seg.inflow_idx)));
        m.add("outflow_idx",
            Values::Value(Values::IntegerValue(seg.outflow_idx)));
        m.add("inflow_lpm",
            Values::Value(Values::DoubleValue(
                sensors[seg.inflow_idx].currentFlow)));
        m.add("outflow_lpm",
            Values::Value(Values::DoubleValue(
                sensors[seg.outflow_idx].currentFlow)));
        m.add("diff_pct",
            Values::Value(Values::DoubleValue(
                static_cast<double>(r.diff_pct))));
        m.add("status",
            Values::Value(Values::StringValue(statusLabel(r.status))));
        m.add("leaked_lpm",
            Values::Value(Values::DoubleValue(
                static_cast<double>(r.leaked_lpm))));
        return m;
    }

    static Values::MapValue buildAlertMap(const PipeSegment& seg)
    {
        const LeakResult& r = seg.last_result;
        Values::MapValue m("segment_id",
            Values::Value(Values::StringValue(seg.segment_id)));
        m.add("status",
            Values::Value(Values::StringValue(statusLabel(r.status))));
        m.add("diff_pct",
            Values::Value(Values::DoubleValue(
                static_cast<double>(r.diff_pct))));
        m.add("leaked_lpm",
            Values::Value(Values::DoubleValue(
                static_cast<double>(r.leaked_lpm))));
        return m;
    }

    // ── sendReading() ─────────────────────────────────────────────────────────
    bool sendReading(const FlowSensor*  sensors,  int sensorCount,
                     const PipeSegment* segments, int segmentCount)
    {
        if (!app.ready())
        {
            Serial.println("[Firebase] Not ready — skipping write.");
            return false;
        }

        // ── Top-level document ────────────────────────────────────────────────
        Document<Values::Value> doc(
            "node_id", Values::Value(Values::StringValue(DEVICE_ID)));
        doc.add("uptime_ms",
            Values::Value(Values::IntegerValue(static_cast<int>(millis()))));

        // ── sensors ArrayValue ────────────────────────────────────────────────
        Values::ArrayValue sensorArr(
            Values::Value(buildSensorMap(1, sensors[0])));
        for (int i = 1; i < sensorCount; i++)
            sensorArr.add(Values::Value(buildSensorMap(i + 1, sensors[i])));
        doc.add("sensors", Values::Value(sensorArr));

        // ── segments ArrayValue ───────────────────────────────────────────────
        if (segmentCount > 0)
        {
            Values::ArrayValue segArr(
                Values::Value(buildSegmentMap(segments[0], sensors)));
            for (int i = 1; i < segmentCount; i++)
                segArr.add(Values::Value(buildSegmentMap(segments[i], sensors)));
            doc.add("segments", Values::Value(segArr));
        }

        // ── alerts ArrayValue (non-NORMAL segments only) ──────────────────────
        //  Build a temp list of alert indices first so we know count upfront.
        int alertIdx[16];  // max 16 segments per node is more than enough
        int alertCount = 0;
        for (int i = 0; i < segmentCount && alertCount < 16; i++)
        {
            if (segments[i].last_result.status != LeakStatus::NORMAL)
                alertIdx[alertCount++] = i;
        }

        if (alertCount > 0)
        {
            Values::ArrayValue alertArr(
                Values::Value(buildAlertMap(segments[alertIdx[0]])));
            for (int i = 1; i < alertCount; i++)
                alertArr.add(Values::Value(
                    buildAlertMap(segments[alertIdx[i]])));
            doc.add("alerts", Values::Value(alertArr));
        }

        // ── Write ─────────────────────────────────────────────────────────────
        String docPath = "flow_readings/";
        docPath += DEVICE_ID;
        docPath += "/readings/";
        docPath += String(millis());

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