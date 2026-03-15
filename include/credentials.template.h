#pragma once

// ─────────────────────────────────────────────────────────────────────────────
//  credentials_template.h  –  COMMIT THIS FILE.
//
//  Copy this file → credentials.h, then fill in your real values.
//  credentials.h is listed in .gitignore and must NEVER be committed.
// ─────────────────────────────────────────────────────────────────────────────

// WiFi
#define WIFI_SSID ""  // e.g. "MyHomeWiFi"
#define WIFI_PASS ""  // e.g. "hunter2"

// Firebase — Web API key (Project Settings → General)
#define FIREBASE_API_KEY ""  // e.g. "AIzaSyXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

// Firebase — Auth user (Authentication → Users, NOT your Google account)
#define FIREBASE_EMAIL ""     // e.g. "device@yourapp.com"
#define FIREBASE_PASSWORD ""  // e.g. "securepassword"

// Firebase — Project ID (Project Settings → General)
#define FIREBASE_PROJECT_ID ""  // e.g. "your-app-12345"

// Identifier written into every Firestore document
#define DEVICE_ID "esp32_node_01"