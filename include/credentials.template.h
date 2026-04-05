#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  credentials_template.h  —  COMMIT THIS FILE  (empty placeholder values)
//  Copy to credentials.h, fill in real values, credentials.h is gitignored.
// ─────────────────────────────────────────────────────────────────────────────

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define WIFI_SSID   "your-ssid"
#define WIFI_PASS   "your-pass"

// ── MQTT broker ───────────────────────────────────────────────────────────────
//  Point this at your Node-RED host (local IP or domain).
//  Node-RED's built-in MQTT broker listens on port 1883 by default.
#define MQTT_BROKER "0.0.0.0"
#define MQTT_PORT   1883
#define MQTT_USER   ""           // leave empty if broker has no auth
#define MQTT_PASS   ""

// ── Daloyko identity ──────────────────────────────────────────────────────────
//  USER_ID must match the userId segment Node-RED expects in the topic path:
//    daloyko/<USER_ID>/<sensorTopic>
#define USER_ID     "your-userid"

//  CLIENT_ID must be unique per ESP32 on the broker (no two nodes the same).
#define MQTT_CLIENT_ID "esp32_node_01"
