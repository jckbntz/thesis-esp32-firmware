#include "real_flow_sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Calibration — YF-S201
//  Manufacturer equation: frequency (Hz) = 7.5 × flow_rate (L/min)
//  Equivalently: 450 pulses accumulate per litre at any flow rate.
//  Tune PULSES_PER_LITRE if your specific unit measures differently.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr float PULSES_PER_LITRE = 450.0f;

namespace RealFlowSensor
{
    // ── Shared ISR ────────────────────────────────────────────────────────────
    //  attachInterruptArg() passes a typed pointer so one function handles
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
            const uint8_t pin = sensors[i].pin;
            // Pins 34/35 are input-only on ESP32 — no internal pull-up.
            const bool inputOnly = (pin == 34 || pin == 35);
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
        const float intervalMin = static_cast<float>(intervalMs) / 60000.0f;

        // Disable interrupts only for the atomic snapshot — keep the window
        // as short as possible so we don't drop pulses on other channels.
        noInterrupts();
        for (int i = 0; i < count; i++)
        {
            const uint32_t pulses = sensors[i].pulseCount;
            sensors[i].pulseCount = 0;

            sensors[i].currentFlow = (intervalMin > 0.0f)
                ? static_cast<double>(pulses / (PULSES_PER_LITRE * intervalMin))
                : 0.0;

            sensors[i].totalLitres =
                static_cast<double>(sensors[i].totalPulses) / PULSES_PER_LITRE;
        }
        interrupts();
    }
}
