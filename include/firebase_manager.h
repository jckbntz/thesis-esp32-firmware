#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  These defines MUST come before FirebaseClient.h in every translation unit
//  that includes this header.  They gate which modules are compiled in.
//  They are also set in platformio.ini build_flags as a safety net.
// ─────────────────────────────────────────────────────────────────────────────
#ifndef ENABLE_FIRESTORE
  #define ENABLE_FIRESTORE
#endif
#ifndef ENABLE_USER_AUTH
  #define ENABLE_USER_AUTH
#endif

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

#include "flow_sensor.h"
#include "credentials.h"

namespace FirebaseManager
{
    // Call once in setup() — after WiFiManager::connectWiFi().
    void begin();

    // Call every loop() iteration — drives token refresh internally.
    void loop();

    // Build and write a 10-sensor Firestore document. Returns true on success.
    bool sendSensors(const FlowSensor* sensors, int count);
}