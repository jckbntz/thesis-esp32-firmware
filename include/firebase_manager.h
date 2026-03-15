#pragma once

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
#include "leak_detector.h"
#include "credentials.h"

namespace FirebaseManager
{
    void begin();
    void loop();

    // Print full reading to Serial — always free, zero quota.
    void printReading(const FlowSensor*  sensors,  int sensorCount,
                      const PipeSegment* segments, int segmentCount);

    // Write structured document to Firestore — costs one write per call.
    // Only call when WRITE_TO_FIRESTORE == 1.
    bool sendReading(const FlowSensor*  sensors,  int sensorCount,
                     const PipeSegment* segments, int segmentCount);
}