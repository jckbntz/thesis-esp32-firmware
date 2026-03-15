#include "real_flow_sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Calibration
//  YF-S201  ≈ 450 pulses / litre
//  Measure with a known volume and tune this value for your exact sensor unit.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr float PULSES_PER_LITRE = 450.0f;

namespace RealFlowSensor
{
    // ── Shared ISR ────────────────────────────────────────────────────────────
    //  attachInterruptArg() passes a typed pointer so one ISR function handles
    //  every channel — no copy-pasted ISR per sensor needed.
    static void IRAM_ATTR onPulse(void* arg)
    {
        FlowSensor* s = static_cast<FlowSensor*>(arg);
        s->pulseCount++;
        s->totalPulses++;
    }

    void begin(FlowSensor* sensors, int count)
    {
        for (int i = 0; i < count; i++)
        {
            const uint8_t pin      = sensors[i].pin;
            // Pins 34/35/36/39 are input-only — no internal pull-up available.
            const bool    inputOnly = (pin == 34 || pin == 35 ||
                                       pin == 36 || pin == 39);
            pinMode(pin, inputOnly ? INPUT : INPUT_PULLUP);

            attachInterruptArg(
                digitalPinToInterrupt(pin),
                onPulse,
                &sensors[i],
                RISING);
        }
        Serial.printf("[RealFlowSensor] %d sensor(s) initialised\n", count);
    }

    void readAndReset(FlowSensor* sensors, int count, unsigned long intervalMs)
    {
        // One critical section — IRQs off only during the atomic snapshot.
        noInterrupts();
        for (int i = 0; i < count; i++)
        {
            const uint32_t pulses  = sensors[i].pulseCount;
            sensors[i].pulseCount  = 0;

            const float intervalMin =
                static_cast<float>(intervalMs) / 60000.0f;

            sensors[i].currentFlow = (intervalMin > 0.0f)
                ? static_cast<double>(pulses / (PULSES_PER_LITRE * intervalMin))
                : 0.0;

            sensors[i].totalLitres =
                static_cast<double>(sensors[i].totalPulses) / PULSES_PER_LITRE;
        }
        interrupts();
    }
}