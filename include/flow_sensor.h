#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  flow_sensor.h
//
//  LeakStatus / LeakResult / PipeSegment have been intentionally removed.
//  Node-RED owns all threshold logic, debounce, and leak classification.
//  The ESP32 is a pure sensor publisher — it reads pulses and reports L/min.
// ─────────────────────────────────────────────────────────────────────────────
#include <Arduino.h>

struct FlowSensor
{
    uint8_t           pin;
    const char*       topic;          // MQTT sub-topic (e.g. "a_in", "main_inlet")
    volatile uint32_t pulseCount;     // ISR-incremented, reset each interval
    uint32_t          totalPulses;    // never reset — lifetime pulse count
    double            currentFlow;    // L/min computed each interval
    double            totalLitres;    // cumulative litres since boot
};
