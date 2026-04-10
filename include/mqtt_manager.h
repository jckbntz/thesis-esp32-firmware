#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  mqtt_manager.h
//
//  Owns the PubSubClient connection to the MQTT broker (Node-RED host).
//  Publishes one message per sensor every interval:
//    Topic:   daloyko/<USER_ID>/<sensor.topic>
//    Payload: float value in L/min, formatted as a plain decimal string
//             e.g.  "3.47"
//
//  Node-RED receives these, runs threshold + debounce logic, and writes the
//  result to Firebase RTDB under:
//    sensors/<USER_ID>/<sensorTopic>  →  { object: <float>, isLeaking: <bool> }
//
//  The ESP32 never touches Firebase directly.
// ─────────────────────────────────────────────────────────────────────────────
#include "flow_sensor.h"

namespace MqttManager
{
    // Connect to broker, set up keep-alive.  Call once in setup().
    void begin();

    // Must be called every loop() iteration — drives the PubSubClient heartbeat
    // and processes any inbound messages (none expected, but required by lib).
    void loop();

    // Publish currentFlow for every sensor in the array.
    // Returns true if all publishes succeeded.
    bool publishAll(const FlowSensor* sensors, int count);

    // Publish a diagnostic heartbeat to daloyko/<USER_ID>/<HEARTBEAT_TOPIC>.
    // Payload: {"uptime":<seconds>, "rssi":<dBm>}
    // Call on its own non-blocking timer in loop() — independent of publishAll().
    bool publishHeartbeat();

    // Print the same data to Serial in human-readable form (zero MQTT cost).
    void printAll(const FlowSensor* sensors, int count);
}