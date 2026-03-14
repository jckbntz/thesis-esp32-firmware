#include "flow_sensor.h"

// ═════════════════════════════════════════════════════════════════════════════
//  SIMULATED FLOW SENSOR
//  Generates a random float in [minVal, maxVal] using esp_random() so it
//  works without the C++ <random> header on Arduino/ESP-IDF.
// ═════════════════════════════════════════════════════════════════════════════
namespace SimulatedFlowSensor
{
    float readFlow(float minVal, float maxVal)
    {
        // esp_random() returns a 32-bit unsigned integer (hardware RNG on ESP32)
        float norm = (float)(esp_random()) / (float)(UINT32_MAX); // 0.0 – 1.0
        return minVal + norm * (maxVal - minVal);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  REAL FLOW SENSOR  (YF-S201 / YF-B10 or any pulse-output sensor)
//
//  Wiring:  VCC → 5 V (or 3.3 V depending on sensor)
//           GND → GND
//           Signal → GPIO pin (3.3 V tolerant)
//
//  Calibration factor (PULSES_PER_LITRE):
//    YF-S201  → ~450 pulses / litre  (7.5 × Hz = L/min in manufacturer's eq.)
//    Adjust to your specific sensor after measuring with a known volume.
// ═════════════════════════════════════════════════════════════════════════════
namespace RealFlowSensor
{
    // ── Calibration ──────────────────────────────────────────────────────────
    static constexpr float PULSES_PER_LITRE = 450.0f; // pulses per litre

    // ── Internal state ────────────────────────────────────────────────────────
    static volatile uint32_t pulseCount1 = 0;
    static volatile uint32_t pulseCount2 = 0;

    // ── ISR callbacks ─────────────────────────────────────────────────────────
    static void IRAM_ATTR onPulse1() { pulseCount1++; }
    static void IRAM_ATTR onPulse2() { pulseCount2++; }

    // ── Public API ────────────────────────────────────────────────────────────
    void begin(uint8_t pinSensor1, uint8_t pinSensor2)
    {
        pinMode(pinSensor1, INPUT_PULLUP);
        pinMode(pinSensor2, INPUT_PULLUP);

        attachInterrupt(digitalPinToInterrupt(pinSensor1), onPulse1, RISING);
        attachInterrupt(digitalPinToInterrupt(pinSensor2), onPulse2, RISING);

        Serial.printf("[FlowSensor] Real sensors initialised on pins %d & %d\n",
                      pinSensor1, pinSensor2);
    }

    void readAndReset(unsigned long intervalMs,
                      float &outSensor1,
                      float &outSensor2)
    {
        // Snapshot and reset atomically (disable IRQ briefly)
        noInterrupts();
        uint32_t p1 = pulseCount1;
        pulseCount1 = 0;
        uint32_t p2 = pulseCount2;
        pulseCount2 = 0;
        interrupts();

        float intervalMin = intervalMs / 60000.0f; // convert ms → minutes

        // L/min = pulses / (PULSES_PER_LITRE * intervalMin)
        outSensor1 = (intervalMin > 0) ? (p1 / (PULSES_PER_LITRE * intervalMin)) : 0.0f;
        outSensor2 = (intervalMin > 0) ? (p2 / (PULSES_PER_LITRE * intervalMin)) : 0.0f;
    }
}