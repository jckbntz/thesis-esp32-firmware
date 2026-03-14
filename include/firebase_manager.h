#pragma once

#include <Firebase_ESP_Client.h>

// ── Firestore collection / document path ───────────────────────────────────
// Writes to:  flow_readings/{deviceId}/readings/{timestamp}
#define DEVICE_ID "esp32_node_01"

namespace FirebaseManager
{
    extern FirebaseData fbdo;
    extern FirebaseAuth auth;
    extern FirebaseConfig config;

    void connectFirebase();

    /**
     * Write both sensor readings to Firestore in a single document.
     * @param flowIn  – FlowSensor1_In  (L/min)
     * @param flowOut – FlowSensor2_Out (L/min)
     * @return true on success
     */
    bool sendFlowReadings(float flowIn, float flowOut);
}