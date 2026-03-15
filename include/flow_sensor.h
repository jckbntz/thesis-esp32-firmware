#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
//  FlowSensor  –  per-channel hardware state
// ─────────────────────────────────────────────────────────────────────────────
struct FlowSensor
{
    uint8_t           pin;
    volatile uint32_t pulseCount;   // reset each interval
    uint32_t          totalPulses;  // running total, never reset
    double            currentFlow;  // L/min computed last interval
    double            totalLitres;  // cumulative litres
};

// ─────────────────────────────────────────────────────────────────────────────
//  LeakStatus  –  result of comparing inflow vs outflow on one segment
//
//  Thresholds derived from YF-S201 specs:
//    Sensor accuracy : ±10%
//    Additional tolerance: ±5%
//    Combined band used:
//      diff% <= 10% → NORMAL     (within sensor accuracy)
//      diff% 10–15% → SUSPECTED  (exceeds accuracy, within accuracy+tolerance)
//      diff% >  15% → CONFIRMED  (exceeds both — unambiguous loss)
//
//  diff% = (inflow - outflow) / inflow × 100
//  leaked_lpm is only populated when status == CONFIRMED.
//  We do not speculate beyond what the sensor data shows.
// ─────────────────────────────────────────────────────────────────────────────
enum class LeakStatus : uint8_t
{
    NORMAL    = 0,  // diff <= 10% — within sensor accuracy
    SUSPECTED = 1,  // diff 10–15% — possible leak, needs monitoring
    CONFIRMED = 2   // diff  > 15% — leak confirmed
};

struct LeakResult
{
    LeakStatus status;
    float      diff_pct;     // (inflow - outflow) / inflow × 100
    float      leaked_lpm;   // populated only when CONFIRMED, else 0
};

// ─────────────────────────────────────────────────────────────────────────────
//  PipeSegment  –  one logical pipe section monitored by exactly two sensors.
//
//  Any number of segments can be defined per ESP32 node.
//  The two sensors must already exist in the main sensors[] array.
//  segment_id is a short label written into the Firestore document so the
//  React frontend can group and display segments without re-implementing logic.
//
//  Example for 3 ESP32 nodes × 10 sensors each (30 total):
//    node_01 → segments A–D  (sensors 1–8, paired as A=1/2, B=3/4, C=5/6, D=7/8)
//    node_01 → sensors 9,10 unpaired (solo monitoring, no leak detection)
//    node_02 → segments E–H  etc.
// ─────────────────────────────────────────────────────────────────────────────
struct PipeSegment
{
    const char* segment_id;   // e.g. "seg_A", "main_inlet", "bathroom_branch"
    int         inflow_idx;   // index into sensors[] for the upstream sensor
    int         outflow_idx;  // index into sensors[] for the downstream sensor
    LeakResult  last_result;  // updated by LeakDetector::evaluate() each interval
};