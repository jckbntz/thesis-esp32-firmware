#include <Arduino.h>
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "flow_sensor.h"
#include "node_selector.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SENSOR MODE
//  0 = simulated random values (safe for bench testing without sensors wired)
//  1 = real YF-S201 sensors via ISR pulse counting
// ─────────────────────────────────────────────────────────────────────────────
#define USE_REAL_SENSORS 1

#if USE_REAL_SENSORS
  #include "real_flow_sensor.h"
#else
  #include "sim_flow_sensor.h"
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  PUBLISH INTERVAL
//  How often the ESP32 reads sensors and publishes to MQTT.
//  Node-RED receives each publish and applies its 5-second debounce on top,
//  so keep this ≤ DEBOUNCE_TIME_MS in the Node-RED function (currently 5000 ms).
//  Recommended: 2000 ms gives Node-RED ~2–3 samples within its debounce window.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr unsigned long PUBLISH_INTERVAL_MS = 2000UL;

static_assert(PUBLISH_INTERVAL_MS >= 500UL,
    "PUBLISH_INTERVAL_MS below 500 ms risks flooding the broker.");

// ─────────────────────────────────────────────────────────────────────────────
//  SENSOR ARRAY  —  built from node_config.h via X-macro expansion
//
//  To add/remove sensors: edit SENSOR_DEFS in node_config.h only.
//  Nothing here needs to change.
// ─────────────────────────────────────────────────────────────────────────────
namespace
{
    // Step 1: count entries by expanding into an enum.
    enum SensorIndex {
        #define X(pin, topic) _SENSOR_IDX_##pin##_ENUM,
        SENSOR_DEFS
        #undef X
        NUM_SENSORS
    };

    static_assert(NUM_SENSORS > 0, "SENSOR_DEFS must define at least one sensor.");

    // Step 2: build the FlowSensor array by expanding the same macro.
    //   { pin, topic, pulseCount=0, totalPulses=0, currentFlow=0, totalLitres=0 }
    static FlowSensor sensors[NUM_SENSORS] = {
        #define X(pin, topic) { pin, topic, 0, 0, 0.0, 0.0 },
        SENSOR_DEFS
        #undef X
    };
}

// ─────────────────────────────────────────────────────────────────────────────
//  TIMING
//  Two completely independent timers — heartbeat never blocks sensor reads.
// ─────────────────────────────────────────────────────────────────────────────
static unsigned long lastPublishTime   = 0;
static unsigned long lastHeartbeatTime = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println("\n════════════════════════════════════════");
    Serial.println("  Daloyko Flow Node — booting");
    Serial.printf ("  Sensors : %d\n", NUM_SENSORS);
    Serial.printf ("  Interval: %lu ms\n", PUBLISH_INTERVAL_MS);
    Serial.printf ("  User ID : %s\n", USER_ID);
    Serial.printf ("  Mode    : %s\n",
        USE_REAL_SENSORS ? "REAL sensors" : "SIMULATED sensors");
    Serial.println("════════════════════════════════════════\n");

    WiFiManager::connectWiFi();
    MqttManager::begin();

#if USE_REAL_SENSORS
    RealFlowSensor::begin(sensors, NUM_SENSORS);
    Serial.println("[Main] Real sensor ISRs attached.\n");
#endif

    lastPublishTime = millis();
}

// ─────────────────────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────────────────────
void loop()
{
    // ── Keep connections alive ────────────────────────────────────────────────
    WiFiManager::maintainConnection();
    MqttManager::loop();

    const unsigned long now = millis();

    if (now - lastPublishTime >= PUBLISH_INTERVAL_MS)
    {
        const unsigned long elapsed = now - lastPublishTime;
        lastPublishTime = now;

        // ── 1. Read sensors ───────────────────────────────────────────────────
#if USE_REAL_SENSORS
        RealFlowSensor::readAndReset(sensors, NUM_SENSORS, elapsed);
#else
        SimulatedFlowSensor::fillAll(sensors, NUM_SENSORS,
                                     0.5f, 15.0f, elapsed);
#endif

        // ── 2. Print to Serial ────────────────────────────────────────────────
        MqttManager::printAll(sensors, NUM_SENSORS);

        // ── 3. Publish to MQTT → Node-RED ─────────────────────────────────────
        const bool ok = MqttManager::publishAll(sensors, NUM_SENSORS);
        if (!ok)
            Serial.println("[Main] ⚠ One or more publishes failed — will retry next tick.");
    }

    // ── Heartbeat — independent 10-second timer ───────────────────────────────
    //  Uses millis() so it never blocks the sensor loop above.
    //  Sends:  daloyko/<USER_ID>/<HEARTBEAT_TOPIC>  →  {"uptime":…, "rssi":…}
    //  Node-RED watchdog timeout is 60 s — this fires every 10 s, giving
    //  5 missed messages of buffer before a false "OFFLINE" alarm triggers.
    if (now - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS)
    {
        lastHeartbeatTime = now;
        MqttManager::publishHeartbeat();
    }
}