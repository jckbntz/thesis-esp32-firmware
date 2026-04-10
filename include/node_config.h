#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  node_config.h  —  SINGLE SOURCE OF TRUTH FOR NODE HARDWARE + MQTT LAYOUT
//
//  Architecture overview:
//    ESP32  →  MQTT  →  Node-RED  →  Firebase RTDB
//
//  The ESP32's ONLY job is to read pulse counts, convert to L/min, and publish
//  raw float values to MQTT.  Node-RED owns ALL threshold logic, debounce,
//  and leak classification.  The ESP32 never decides whether a leak exists.
//
//  Topic schema (must match the Node-RED function node exactly):
//    daloyko/<USER_ID>/main_inlet          ← one main inlet sensor
//    daloyko/<USER_ID>/<zone>_in           ← zone inflow sensors
//    daloyko/<USER_ID>/<zone>_out          ← zone outflow sensors
//
//  To scale this node:
//    1. Edit SENSOR_DEFS — one row per physical sensor
//    2. That's it.  Everything else adapts automatically.
//
//  SENSOR_DEFS columns:
//    ( gpio_pin,  mqtt_subtopic )
//
//  gpio_pin      — ESP32 GPIO the YF-S201 signal wire is connected to.
//                  Pins 34/35/36/39 are INPUT-ONLY — no INPUT_PULLUP.
//  mqtt_subtopic — the third path segment of the MQTT topic.
//                  Use exactly "main_inlet" for the main meter.
//                  Use "<zone>_in" / "<zone>_out" for zone pairs.
//                  Zone names can be anything (a, b, kitchen, bathroom …).
// ─────────────────────────────────────────────────────────────────────────────

// ── Heartbeat topic suffix ────────────────────────────────────────────────────
//  Full topic published: daloyko/<USER_ID>/<HEARTBEAT_TOPIC>
//  Must be unique per ESP32 node so Node-RED can tell them apart.
//  Change the suffix for each node (heartbeat_ESP32_2, heartbeat_ESP32_3 …).
#define HEARTBEAT_TOPIC "heartbeat_ESP32_4"

// ── Heartbeat interval ────────────────────────────────────────────────────────
//  Send every 10 s.  Node-RED watchdog fires after 60 s — gives 5 missed
//  heartbeats of buffer before a false "OFFLINE" alarm triggers.
#define HEARTBEAT_INTERVAL_MS 10000UL

#define SENSOR_DEFS \
    X(35, "Riser1_n02_in")       \
    X(32, "Floor2_n01_in")       \
    X(33, "Floor2_n02_in")       \
    X(21, "Riser1_n02_out")      \
    X(19, "Floor2_n01_out")      \
    X(18, "Floor2_n02_out")
    // X(34, "main_inlet") // FOR ESP 1 ONLY
//  ↑ Add or remove rows freely.
//    Each row = one physical YF-S201 sensor.
//    The order here becomes the sensor index (0, 1, 2 …).