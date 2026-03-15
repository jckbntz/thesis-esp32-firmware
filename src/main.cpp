#include <Arduino.h>
#include "credentials.h"
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "flow_sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SENSOR MODE
//  0 = simulated random values (safe for testing without hardware)
//  1 = real YF-S201 / YF-B10 sensors on the pins defined below
// ─────────────────────────────────────────────────────────────────────────────
#define USE_REAL_SENSORS 0

// Conditionally include only the sensor driver that is actually used.
// This prevents the unused driver's symbols from being compiled in.
#if USE_REAL_SENSORS
  #include "real_flow_sensor.h"
#else
  #include "sim_flow_sensor.h"
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  SENSOR ARRAY  –  10 channels for this node
//
//  ⚠ Pins 34, 35, 36, 39 are INPUT-ONLY on the ESP32.
//    Do NOT use INPUT_PULLUP on those pins.
//    Use an external 10 kΩ pull-up resistor or choose a different pin.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int NUM_SENSORS = 10;

static FlowSensor sensors[NUM_SENSORS] = {
    {34, 0, 0, 0.0, 0.0}, {35, 0, 0, 0.0, 0.0}, {36, 0, 0, 0.0, 0.0},
    {39, 0, 0, 0.0, 0.0}, { 4, 0, 0, 0.0, 0.0}, {13, 0, 0, 0.0, 0.0},
    {14, 0, 0, 0.0, 0.0}, {16, 0, 0, 0.0, 0.0}, {17, 0, 0, 0.0, 0.0},
    {18, 0, 0, 0.0, 0.0}
};

// ─────────────────────────────────────────────────────────────────────────────
//  SEND INTERVAL
//
//  Firestore free tier: 20,000 writes / day
//    Every  5 s → 17,280 writes/day  ✓  comfortable margin
//    Every  3 s → 28,800 writes/day  ✗  exceeds free quota
//    Every 10 s →  8,640 writes/day  ✓  very safe
//
//  Change SEND_INTERVAL_MS to 5000 or higher for production use.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr unsigned long SEND_INTERVAL_MS = 5000UL;

static_assert(SEND_INTERVAL_MS >= 5000,
    "SEND_INTERVAL_MS must be >= 5000 ms to stay within the "
    "Firestore free-plan 20,000 writes/day limit.");

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
    FirebaseManager::begin();

#if USE_REAL_SENSORS
    RealFlowSensor::begin(sensors, NUM_SENSORS);
    Serial.println("[Main] Mode: REAL sensors");
#else
    Serial.println("[Main] Mode: SIMULATED sensors (random values)");
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
//  LOOP  –  non-blocking timer, no delay(), no RTOS tasks needed
// ─────────────────────────────────────────────────────────────────────────────
void loop()
{
    // FirebaseManager::loop() drives auth token refresh every iteration.
    // Never remove this call or the token will expire after ~1 hour.
    FirebaseManager::loop();

    const unsigned long now = millis();

    if (now - lastSendTime >= SEND_INTERVAL_MS)
    {
        const unsigned long elapsed = now - lastSendTime;
        lastSendTime = now;

#if USE_REAL_SENSORS
        // Atomically snapshot and reset pulse counters, then compute L/min.
        RealFlowSensor::readAndReset(sensors, NUM_SENSORS, elapsed);
#else
        // Fill all slots with random values in a realistic range.
        SimulatedFlowSensor::fillAll(sensors, NUM_SENSORS,
                                     0.5f, 15.0f, elapsed);
#endif

        // Log all channels to Serial
        for (int i = 0; i < NUM_SENSORS; i++)
        {
            Serial.printf("[Sensor %2d] pin %2d | flow: %5.2f L/min | total: %7.2f L\n",
                          i + 1,
                          sensors[i].pin,
                          sensors[i].currentFlow,
                          sensors[i].totalLitres);
        }

        // Write the full 10-sensor document to Firestore
        FirebaseManager::sendSensors(sensors, NUM_SENSORS);
    }

    // ── Add other non-blocking tasks here ────────────────────────────────────
}