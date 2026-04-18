#include "real_flow_sensor.h"

static constexpr float PULSES_PER_LITRE = 450.0f;
static constexpr float MIN_FLOW_THRESHOLD = 0.1f;

namespace RealFlowSensor
{
    struct ISRContext {
        FlowSensor* sensor;
    };

    static ISRContext contexts[25];

    // ── Clean ISR — no debounce, no micros(), matches working sketch ──────────
    static void IRAM_ATTR onPulseSafe(void* arg)
    {
        ISRContext* ctx = static_cast<ISRContext*>(arg);
        ctx->sensor->pulseCount++;
        ctx->sensor->totalPulses++;
    }

    void begin(FlowSensor* sensors, int count)
    {
        for (int i = 0; i < count; i++)
        {
            const uint8_t pin = sensors[i].pin;
            const bool inputOnly = (pin >= 34 && pin <= 39);
            pinMode(pin, inputOnly ? INPUT : INPUT_PULLUP);

            contexts[i].sensor = &sensors[i];

            attachInterruptArg(
                digitalPinToInterrupt(pin),
                onPulseSafe,
                &contexts[i],
                RISING);
        }

        Serial.printf("[RealFlowSensor] %d sensor(s) initialised\n", count);
    }

    void readAndReset(FlowSensor* sensors, int count, unsigned long intervalMs)
    {
        // ── Critical section: snapshot + reset counters only ─────────────────
        // Keep this window as short as possible — no math here.
        uint32_t snapshots[25] = {};

        noInterrupts();
        for (int i = 0; i < count; i++) {
            snapshots[i] = sensors[i].pulseCount;
            sensors[i].pulseCount = 0;
        }
        interrupts();

        // ── All computation happens outside the critical section ──────────────
        const float intervalMin = static_cast<float>(intervalMs) / 60000.0f;

        for (int i = 0; i < count; i++)
        {
            float flow = (intervalMin > 0.0f)
                ? (snapshots[i] / (PULSES_PER_LITRE * intervalMin))
                : 0.0f;

            if (flow < MIN_FLOW_THRESHOLD)
                flow = 0.0f;

            sensors[i].currentFlow  = flow;
            sensors[i].totalLitres  =
                static_cast<double>(sensors[i].totalPulses) / PULSES_PER_LITRE;
        }
    }
}