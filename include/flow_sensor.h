#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Flow-sensor abstraction
//  Use SimulatedFlowSensor while you have no hardware.
//  Swap in RealFlowSensor when your pulse-output sensors are wired up.
// ─────────────────────────────────────────────────────────────────────────────

// ── Simulated (random values, no hardware needed) ────────────────────────────
namespace SimulatedFlowSensor
{
    /**
     * Returns a random flow rate between minVal and maxVal (L/min).
     * Call this in place of a real sensor read.
     */
    float readFlow(float minVal = 0.5f, float maxVal = 15.0f);
}

// ── Real pulse-output flow sensor (YF-S201 or similar) ───────────────────────
namespace RealFlowSensor
{
    /**
     * Call once in setup() for each sensor pin.
     * Attaches the interrupt that counts pulses.
     */
    void begin(uint8_t pinSensor1, uint8_t pinSensor2);

    /**
     * Must be called regularly (e.g. every 3 s) to convert accumulated
     * pulse counts into L/min, then reset the counters.
     *
     * @param intervalMs   – milliseconds since last call (use your timer delta)
     * @param outSensor1   – result for FlowSensor1_In
     * @param outSensor2   – result for FlowSensor2_Out
     */
    void readAndReset(unsigned long intervalMs,
                      float &outSensor1,
                      float &outSensor2);
}