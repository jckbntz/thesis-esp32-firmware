#pragma once
#include "flow_sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  RealFlowSensor
//
//  Supports YF-S201, YF-B10, or any rising-edge pulse-output sensor.
//
//  Wiring:
//    VCC    → 5 V (or 3.3 V — check your sensor datasheet)
//    GND    → GND
//    Signal → interrupt-capable GPIO (3.3 V tolerant)
//
//  ⚠ ESP32 pins 34, 35, 36, 39 are INPUT-ONLY.
//    INPUT_PULLUP will not work on those pins.
//    Use an external 10 kΩ pull-up, or choose a different pin.
// ─────────────────────────────────────────────────────────────────────────────
namespace RealFlowSensor
{
    // Attach interrupts for all sensors in the array.
    // Call once in setup() after Serial is ready.
    void begin(FlowSensor* sensors, int count);

    // Atomically snapshot + reset pulse counters, then recompute flow metrics.
    // Call once per SEND_INTERVAL from loop(), passing the exact elapsed ms.
    void readAndReset(FlowSensor* sensors, int count, unsigned long intervalMs);
}