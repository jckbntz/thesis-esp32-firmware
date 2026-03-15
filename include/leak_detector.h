#pragma once
#include "flow_sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  LeakDetector
//
//  Compares inflow vs outflow for every PipeSegment using the combined
//  sensor accuracy (±10%) and tolerance (±5%) of the YF-S201.
//
//  Rules:
//    1. If inflow < 0.5 L/min both sensors are effectively idle — skip.
//       Comparing two near-zero readings produces meaningless diff%.
//    2. diff% = (inflow - outflow) / inflow × 100
//    3. Negative diff (outflow > inflow) is clamped to 0 — physically impossible
//       on a closed segment; indicates sensor noise, not reverse flow.
//    4. Status thresholds:
//         diff <= THRESHOLD_NORMAL    → NORMAL
//         diff <= THRESHOLD_CONFIRMED → SUSPECTED
//         diff >  THRESHOLD_CONFIRMED → CONFIRMED
//    5. leaked_lpm is only set when CONFIRMED.
//       We never speculate volume on SUSPECTED.
// ─────────────────────────────────────────────────────────────────────────────

namespace LeakDetector
{
    // Threshold constants — adjust here if using a different sensor model.
    static constexpr float THRESHOLD_NORMAL    = 10.0f; // % — within sensor accuracy
    static constexpr float THRESHOLD_CONFIRMED = 15.0f; // % — beyond accuracy+tolerance
    static constexpr float MIN_FLOW_LPM        =  0.5f; // L/min — below this, skip eval

    // Evaluate all segments in the array and update their last_result.
    // Call once per interval after sensor readings are computed.
    void evaluateAll(PipeSegment* segments, int segCount,
                     const FlowSensor* sensors);

    // Evaluate a single segment. Returns the result and also writes it into
    // segment.last_result for convenience.
    LeakResult evaluate(PipeSegment& segment, const FlowSensor* sensors);
}