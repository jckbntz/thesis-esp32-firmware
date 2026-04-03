#include "sim_flow_sensor.h"

namespace SimulatedFlowSensor
{
    void fillAll(FlowSensor* sensors, int count,
                 float minVal, float maxVal,
                 unsigned long intervalMs)
    {
        const float intervalMin =
            static_cast<float>(intervalMs) / 60000.0f;

        for (int i = 0; i < count; i++)
        {
            // esp_random() — 32-bit hardware RNG, uniform distribution
            const float norm =
                static_cast<float>(esp_random()) /
                static_cast<float>(UINT32_MAX);   // 0.0 – 1.0

            sensors[i].currentFlow =
                static_cast<double>(minVal + norm * (maxVal - minVal));

            // Accumulate: flow (L/min) × elapsed (min) = litres added this tick
            sensors[i].totalLitres += sensors[i].currentFlow * intervalMin;
        }
    }
}
