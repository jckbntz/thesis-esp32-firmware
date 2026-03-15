#pragma once
#include "flow_sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SimulatedFlowSensor
//
//  Fills every sensor slot with a plausible random flow value using the
//  ESP32's hardware RNG (esp_random).  No pins, no interrupts.
//  Used when USE_REAL_SENSORS == 0 in main.cpp.
// ─────────────────────────────────────────────────────────────────────────────
namespace SimulatedFlowSensor
{
    // Randomise currentFlow for every sensor in [minVal, maxVal] L/min and
    // accumulate totalLitres based on the elapsed interval.
    void fillAll(FlowSensor* sensors, int count,
                 float minVal, float maxVal,
                 unsigned long intervalMs);
}