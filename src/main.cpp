#include <Arduino.h>
#include "credentials.h"
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "flow_sensor.h"
#include "leak_detector.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SENSOR MODE
//  0 = simulated random values   1 = real YF-S201 sensors
// ─────────────────────────────────────────────────────────────────────────────
#define USE_REAL_SENSORS 0

#if USE_REAL_SENSORS
  #include "real_flow_sensor.h"
#else
  #include "sim_flow_sensor.h"
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  WRITE STRATEGY
//
//  WRITE_TO_FIRESTORE  0 = Serial only (development)
//                      1 = event-driven writes (production)
//
//  When 1, Firestore is written in exactly two cases:
//    a) A leak is detected (SUSPECTED or CONFIRMED on any segment)
//    b) A periodic heartbeat fires (every HEARTBEAT_INTERVAL_MS)
//
//  Quota estimates — 3 nodes total:
//    Leaking all day : 3 × 17,280 = 51,840 writes/day  (Blaze tier)
//    No leaks        : 3 ×    144 =    432 writes/day  (well within Spark free)
//    Heartbeat only  : 3 ×     48 =    144 writes/day  (30-min interval)
// ─────────────────────────────────────────────────────────────────────────────
#define WRITE_TO_FIRESTORE 0

// Heartbeat interval — one write to confirm the node is alive (ms).
// 600000 = 10 min (144/day).  Use 1800000 (30 min) to be very conservative.
static constexpr unsigned long HEARTBEAT_INTERVAL_MS = 600000UL;

// ─────────────────────────────────────────────────────────────────────────────
//  SENSOR ARRAY  –  10 sensors on this node
//
//  ⚠ Pins 34, 35, 36, 39 are INPUT-ONLY on ESP32.
//    Do NOT use INPUT_PULLUP on those pins.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int NUM_SENSORS = 10;

static FlowSensor sensors[NUM_SENSORS] = {
    {34, 0, 0, 0.0, 0.0},   // index 0
    {35, 0, 0, 0.0, 0.0},   // index 1
    {36, 0, 0, 0.0, 0.0},   // index 2
    {39, 0, 0, 0.0, 0.0},   // index 3
    { 4, 0, 0, 0.0, 0.0},   // index 4
    {13, 0, 0, 0.0, 0.0},   // index 5
    {14, 0, 0, 0.0, 0.0},   // index 6
    {16, 0, 0, 0.0, 0.0},   // index 7
    {17, 0, 0, 0.0, 0.0},   // index 8
    {18, 0, 0, 0.0, 0.0}    // index 9
};

// ─────────────────────────────────────────────────────────────────────────────
//  PIPE SEGMENTS  –  paired sensors on the same physical pipe section
//
//  Adjust segment_id names and index pairs to match your installation.
//    seg_A  →  sensors 0/1  (pins 34/35) — main inlet
//    seg_B  →  sensors 2/3  (pins 36/39) — branch 1
//    seg_C  →  sensors 4/5  (pins  4/13) — branch 2
//    seg_D  →  sensors 6/7  (pins 14/16) — branch 3
//    sensors 8, 9 (pins 17, 18) — solo monitoring, no leak pairing
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int NUM_SEGMENTS = 4;

static PipeSegment segments[NUM_SEGMENTS] = {
    { "seg_A", 0, 1, {LeakStatus::NORMAL, 0.f, 0.f} },
    { "seg_B", 2, 3, {LeakStatus::NORMAL, 0.f, 0.f} },
    { "seg_C", 4, 5, {LeakStatus::NORMAL, 0.f, 0.f} },
    { "seg_D", 6, 7, {LeakStatus::NORMAL, 0.f, 0.f} },
};

// ─────────────────────────────────────────────────────────────────────────────
//  INTERVALS
// ─────────────────────────────────────────────────────────────────────────────
static constexpr unsigned long SEND_INTERVAL_MS = 5000UL;

static_assert(SEND_INTERVAL_MS >= 5000,
    "SEND_INTERVAL_MS must be >= 5000 ms to stay within Firestore free quota.");

// ─────────────────────────────────────────────────────────────────────────────
//  GLOBALS
// ─────────────────────────────────────────────────────────────────────────────
static unsigned long lastSendTime      = 0;
static unsigned long lastHeartbeatTime = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────────────────────────────────────
static bool hasActiveLeak()
{
    for (int i = 0; i < NUM_SEGMENTS; i++)
        if (segments[i].last_result.status != LeakStatus::NORMAL)
            return true;
    return false;
}

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

    Serial.printf("[Main] Segments: %d  |  Leak detection: active\n",
                  NUM_SEGMENTS);
    Serial.printf("[Main] Write strategy: %s\n",
#if WRITE_TO_FIRESTORE
                  "event-driven (leak + heartbeat)"
#else
                  "Serial only (WRITE_TO_FIRESTORE=0)"
#endif
    );
}

// ─────────────────────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────────────────────
void loop()
{
    FirebaseManager::loop();  // drives token refresh — never remove

    const unsigned long now = millis();

    if (now - lastSendTime >= SEND_INTERVAL_MS)
    {
        const unsigned long elapsed = now - lastSendTime;
        lastSendTime = now;

        // ── 1. Read sensors ───────────────────────────────────────────────────
#if USE_REAL_SENSORS
        RealFlowSensor::readAndReset(sensors, NUM_SENSORS, elapsed);
#else
        SimulatedFlowSensor::fillAll(sensors, NUM_SENSORS,
                                     0.5f, 15.0f, elapsed);
#endif

        // ── 2. Run leak detection on all segments ─────────────────────────────
        LeakDetector::evaluateAll(segments, NUM_SEGMENTS, sensors);

        // ── 3. Print full reading to Serial — always, always free ─────────────
        FirebaseManager::printReading(sensors,  NUM_SENSORS,
                                      segments, NUM_SEGMENTS);

#if WRITE_TO_FIRESTORE
        // ── 4a. Leak write — triggers immediately on any SUSPECTED/CONFIRMED ──
        //  Every 5 s interval that has an active leak writes one document.
        //  React gets a live alert stream at full resolution during a leak event.
        if (hasActiveLeak())
        {
            Serial.println("[Main] Leak detected → writing to Firestore.");
            FirebaseManager::sendReading(sensors,  NUM_SENSORS,
                                         segments, NUM_SEGMENTS);
            lastHeartbeatTime = now;  // reset so we don't double-write
        }
        // ── 4b. Heartbeat — periodic write when all segments are normal ───────
        //  Lets the React dashboard show "last seen X min ago" per node.
        //  Also catches a node going offline — if the last heartbeat is old,
        //  the frontend can raise a connectivity alert.
        else if (now - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS)
        {
            lastHeartbeatTime = now;
            Serial.println("[Main] Heartbeat → writing to Firestore.");
            FirebaseManager::sendReading(sensors,  NUM_SENSORS,
                                         segments, NUM_SEGMENTS);
        }
        else
        {
            // Normal — no leak, heartbeat not due yet. Zero writes this interval.
            const unsigned long secsUntilHeartbeat =
                (HEARTBEAT_INTERVAL_MS - (now - lastHeartbeatTime)) / 1000UL;
            Serial.printf("[Main] Normal — next heartbeat in %lu s\n",
                          secsUntilHeartbeat);
        }
#else
        Serial.println("[Main] Firestore write skipped (WRITE_TO_FIRESTORE=0)");
#endif
    }
}