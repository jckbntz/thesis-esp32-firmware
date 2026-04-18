#pragma once
#define HEARTBEAT_TOPIC "heartbeat_ESP32_1"
#define HEARTBEAT_INTERVAL_MS 10000UL

#define SENSOR_DEFS\
    X(32, "Floor2_n01_in")\
    X(33, "Floor2_n01_out")\
    X(25, "Floor2_n02_in")\
    X(26, "Floor2_n02_out")\
    X(27, "Floor2_n03_in")\
    X(5,  "Floor2_n03_out")\
    X(17, "Floor2_n04_in")\
    X(16, "Floor2_n04_out")\
    X(4,  "Floor2_n05_in")\
    X(15, "Floor2_n05_out")