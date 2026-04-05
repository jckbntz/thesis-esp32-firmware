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
//                  Pins 34/35 are INPUT-ONLY — no INPUT_PULLUP.
//  mqtt_subtopic — the third path segment of the MQTT topic.
//                  Use exactly "main_inlet" for the main meter.
//                  Use "<zone>_in" / "<zone>_out" for zone pairs.
//                  Zone names can be anything (a, b, kitchen, bathroom …).
// ─────────────────────────────────────────────────────────────────────────────

#define SENSOR_DEFS \
    X(34, "main_inlet") \
    X(35, "Floor1_a_in")       \
    X( 5, "Floor1_a_out")      \
    X(15, "Floor1_b_in")       \
    X( 4, "Floor1_b_out")      \
    X(13, "Floor1_c_in")       \
    X(14, "Floor1_c_out")      \
    X(16, "Floor1_d_in")       \
    X(17, "Floor1_d_out")      \
    X(18, "Floor1_e_in")       \
    X(19, "Floor1_e_out")      \
    X(21, "Floor1_f_in")       \
    X(22, "Floor1_f_out")      \
    X(23, "Floor1_g_in")       \
    X(25, "Floor1_g_out")      \
    X(26, "Floor1_h_in")       \
    X(27, "Floor1_h_out")      \
    X(32, "Floor1_i_in")       \
    X(33, "Floor1_i_out")
//  ↑ Add or remove rows freely.
//    Each row = one physical YF-S201 sensor.
//    The order here becomes the sensor index (0, 1, 2 …).