#pragma once

#include <Firebase_ESP_Client.h>

// ── Credentials (fill in your values) ──────────────────────────────────────
#define FIREBASE_API_KEY "YOUR_API_KEY"
#define FIREBASE_AUTH_EMAIL "your@email.com"
#define FIREBASE_AUTH_PASS "yourPassword"
#define FIREBASE_PROJECT_ID "your-project-id" // used for Firestore

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