#pragma once
#define HEARTBEAT_TOPIC "heartbeat_ESP32_1"
#define HEARTBEAT_INTERVAL_MS 10000UL

// 33, WHITE
// 32, ORANGE
// 27, GRAY
// 26, YELLOW
// 25, GREEN
// 23, BLUE
// 22, VIOLET

#define SENSOR_DEFS                 \
    X(33, "main_inlet")             \
    X(32, "Floor1_n01_in")          \
    X(27, "Floor1_n02_in")          \
    X(26, "Floor1_n03_in")          \
    X(25, "Floor1_n01_out")         \
    X(23, "Floor1_n02_out")         \
    X(22, "Floor1_n03_out")