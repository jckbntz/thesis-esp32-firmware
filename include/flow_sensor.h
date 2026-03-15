#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
//  FlowSensor  –  per-channel runtime state.
//  Shared between real_flow_sensor.h, sim_flow_sensor.h, firebase_manager.h
//  and main.cpp.  No logic lives here — only the data layout.
// ─────────────────────────────────────────────────────────────────────────────
struct FlowSensor
{
    uint8_t           pin;
    volatile uint32_t pulseCount;   // reset each interval
    uint32_t          totalPulses;  // running hardware total, never reset
    double            currentFlow;  // L/min computed last interval
    double            totalLitres;  // cumulative litres
};