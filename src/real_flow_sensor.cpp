#include "real_flow_sensor.h"

// ─────────────────────────────────────────────
// Calibration (YF-S201)
static constexpr float PULSES_PER_LITRE = 450.0f;

// Debounce threshold (microseconds)
// 1000 µs = 1 ms → ignores high-frequency noise
static constexpr uint32_t DEBOUNCE_US = 1000;

// Minimum valid flow (L/min)
// Values below this are treated as zero (noise floor)
static constexpr float MIN_FLOW_THRESHOLD = 0.1f;

namespace RealFlowSensor
{
    // ── Extended sensor runtime state ─────────────────────────────
    struct SensorRuntime {
        uint32_t lastPulseMicros;
    };

    static SensorRuntime runtime[25]; // supports up to 25 sensors

    // ── ISR with per-sensor debounce ──────────────────────────────
    static void IRAM_ATTR onPulse(void* arg)
    {
        FlowSensor* s = static_cast<FlowSensor*>(arg);

        // Compute index (pointer arithmetic)
        int index = s - &s[0]; // ⚠️ WRONG if used directly → fixed below
    }

    // Correct ISR using pointer offset passed via arg
    struct ISRContext {
        FlowSensor* sensor;
        SensorRuntime* rt;
    };

    static void IRAM_ATTR onPulseSafe(void* arg)
    {
        ISRContext* ctx = static_cast<ISRContext*>(arg);

        uint32_t now = micros();

        if (now - ctx->rt->lastPulseMicros > DEBOUNCE_US)
        {
            ctx->sensor->pulseCount++;
            ctx->sensor->totalPulses++;
            ctx->rt->lastPulseMicros = now;
        }
    }

    // ── Store ISR contexts ────────────────────────────────────────
    static ISRContext contexts[25];

    void begin(FlowSensor* sensors, int count)
    {
        for (int i = 0; i < count; i++)
        {
            const uint8_t pin = sensors[i].pin;

            // GPIO 34–39 = input only → NO pull-up
            const bool inputOnly = (pin >= 34 && pin <= 39);

            pinMode(pin, inputOnly ? INPUT : INPUT_PULLUP);

            // Init runtime
            runtime[i].lastPulseMicros = 0;

            // Setup ISR context
            contexts[i].sensor = &sensors[i];
            contexts[i].rt = &runtime[i];

            attachInterruptArg(
                digitalPinToInterrupt(pin),
                onPulseSafe,
                &contexts[i],
                RISING);
        }

        Serial.printf("[RealFlowSensor] %d sensor(s) initialised (debounced)\n", count);
    }

    void readAndReset(FlowSensor* sensors, int count, unsigned long intervalMs)
    {
        const float intervalMin = static_cast<float>(intervalMs) / 60000.0f;

        noInterrupts();

        for (int i = 0; i < count; i++)
        {
            const uint32_t pulses = sensors[i].pulseCount;
            sensors[i].pulseCount = 0;

            float flow = (intervalMin > 0.0f)
                ? (pulses / (PULSES_PER_LITRE * intervalMin))
                : 0.0f;

            // Noise filter (remove ghost readings)
            if (flow < MIN_FLOW_THRESHOLD)
                flow = 0.0f;

            sensors[i].currentFlow = flow;

            sensors[i].totalLitres =
                static_cast<double>(sensors[i].totalPulses) / PULSES_PER_LITRE;
        }

        interrupts();
    }
}