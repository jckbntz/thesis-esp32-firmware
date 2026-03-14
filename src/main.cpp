#include <Arduino.h>
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "flow_sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  CONFIGURATION
// ─────────────────────────────────────────────────────────────────────────────

// Set to 1 to use real hardware sensors, 0 to use simulated random values.
#define USE_REAL_SENSORS 0

// Pins used when USE_REAL_SENSORS == 1
#define FLOW_PIN_1 GPIO_NUM_18 // FlowSensor1_In
#define FLOW_PIN_2 GPIO_NUM_19 // FlowSensor2_Out

// How often to sample + upload (milliseconds)
static constexpr unsigned long SEND_INTERVAL_MS = 5000UL;

// ─────────────────────────────────────────────────────────────────────────────
//  FREE-PLAN WRITE-BUDGET GUARD
//
//  Firestore free tier: 20,000 writes/day
//  At one write every 3 s that is 28,800 writes/day — over budget!
//
//  Safe intervals:
//    Every  5 s  →  17,280 writes/day  ✓  (comfortable margin)
//    Every  3 s  →  28,800 writes/day  ✗  (exceeds limit)
//    Every 10 s  →   8,640 writes/day  ✓  (very safe)
//
//  ► Change SEND_INTERVAL_MS to 5000 or higher for production use.
// ─────────────────────────────────────────────────────────────────────────────
static_assert(SEND_INTERVAL_MS >= 5000,
              "SEND_INTERVAL_MS must be >= 5000 ms to stay within the Firestore "
              "free-plan 20,000 writes/day limit. "
              "3 s would produce ~28,800 writes/day.");

// ─────────────────────────────────────────────────────────────────────────────
//  GLOBALS
// ─────────────────────────────────────────────────────────────────────────────
static unsigned long lastSendTime = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
  delay(1000);

  WiFiManager::connectWiFi();
  FirebaseManager::connectFirebase();

#if USE_REAL_SENSORS
  RealFlowSensor::begin(FLOW_PIN_1, FLOW_PIN_2);
  Serial.println("[Main] Mode: REAL sensors");
#else
  Serial.println("[Main] Mode: SIMULATED sensors (random values)");
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
//  LOOP  – non-blocking timer (no delay, no threads needed on ESP32)
// ─────────────────────────────────────────────────────────────────────────────
void loop()
{
  unsigned long now = millis();

  if (now - lastSendTime >= SEND_INTERVAL_MS)
  {
    unsigned long elapsed = now - lastSendTime; // exact interval for real sensor
    lastSendTime = now;

    float flowIn = 0.0f;
    float flowOut = 0.0f;

#if USE_REAL_SENSORS
    // ── Real sensors ──────────────────────────────────────────────────
    RealFlowSensor::readAndReset(elapsed, flowIn, flowOut);
#else
    // ── Simulated sensors ─────────────────────────────────────────────
    flowIn = SimulatedFlowSensor::readFlow(0.5f, 15.0f);
    flowOut = SimulatedFlowSensor::readFlow(0.5f, 14.0f); // slightly less than in
#endif

    Serial.printf("[Sensors] In: %.2f L/min  |  Out: %.2f L/min\n",
                  flowIn, flowOut);

    FirebaseManager::sendFlowReadings(flowIn, flowOut);
  }

  // ── Put your other non-blocking tasks here ────────────────────────────
}