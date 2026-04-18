#pragma once
#define HEARTBEAT_TOPIC "heartbeat_ESP32_2"
#define HEARTBEAT_INTERVAL_MS 10000UL

#define SENSOR_DEFS\
    X(32, "main_inlet")\
    X(33, "Floor1_n06_in")\
    X(25, "Floor1_n06_out")\
    X(26, "Floor1_n07_in")\
    X(27, "Floor1_n07_out")         