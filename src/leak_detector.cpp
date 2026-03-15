#include "leak_detector.h"

namespace LeakDetector
{
    LeakResult evaluate(PipeSegment& segment, const FlowSensor* sensors)
    {
        LeakResult result = { LeakStatus::NORMAL, 0.0f, 0.0f };

        const float inflow  = static_cast<float>(sensors[segment.inflow_idx ].currentFlow);
        const float outflow = static_cast<float>(sensors[segment.outflow_idx].currentFlow);

        // ── Rule 1: skip idle segments ────────────────────────────────────────
        // Both sensors reading below minimum means no meaningful water is flowing.
        // Comparing near-zero values produces unstable diff% — do not evaluate.
        if (inflow < MIN_FLOW_LPM)
        {
            segment.last_result = result;
            return result;
        }

        // ── Rule 2: compute diff% ─────────────────────────────────────────────
        float diff = inflow - outflow;

        // ── Rule 3: clamp negative diff ───────────────────────────────────────
        // Outflow > inflow on a closed pipe segment is physically impossible.
        // This is sensor noise — treat as zero difference.
        if (diff < 0.0f) diff = 0.0f;

        const float diff_pct = (diff / inflow) * 100.0f;

        result.diff_pct = diff_pct;

        // ── Rule 4 & 5: classify and conditionally set leaked_lpm ─────────────
        if (diff_pct <= THRESHOLD_NORMAL)
        {
            result.status     = LeakStatus::NORMAL;
            result.leaked_lpm = 0.0f;
        }
        else if (diff_pct <= THRESHOLD_CONFIRMED)
        {
            result.status     = LeakStatus::SUSPECTED;
            result.leaked_lpm = 0.0f;  // do not speculate volume on SUSPECTED
        }
        else
        {
            result.status     = LeakStatus::CONFIRMED;
            result.leaked_lpm = diff;  // only report volume when confirmed
        }

        segment.last_result = result;
        return result;
    }

    void evaluateAll(PipeSegment* segments, int segCount,
                     const FlowSensor* sensors)
    {
        for (int i = 0; i < segCount; i++)
            evaluate(segments[i], sensors);
    }
}