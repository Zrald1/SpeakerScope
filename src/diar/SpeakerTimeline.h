#pragma once

#include <array>
#include <deque>
#include <mutex>
#include <vector>
#include "diar/DiarizationEngine.h"

namespace ss {

// Accumulates ActivityFrames and materializes SpeakerTurns on top of them.
// Keeps a rolling window of raw probability frames (for the attributor's
// argmax queries) plus a debounced, merged turn list.
class SpeakerTimeline {
public:
    struct Options {
        float active_threshold = 0.5f;   // prob above this = channel active
        uint64_t min_turn_ms = 250;      // drop blips shorter than this
        uint64_t merge_gap_ms = 300;     // merge same-speaker turns closer than this
        uint64_t window_ms = 120'000;    // keep 2 min of prob frames
        uint64_t close_guard_ms = 500;   // don't close turns this close to live edge
    };

    SpeakerTimeline() = default;
    explicit SpeakerTimeline(Options opts) : opts_(opts) {}

    void addFrame(const ActivityFrame& f);  // called from diarization worker
    // Direct-turn path for engines that report decoded SpeakerTurns rather
    // than prob frames (audio.cpp nemotron_3_diar). Inserts sorted; merges
    // same-speaker turns within merge_gap_ms. Safe for late-arriving turns.
    void addTurn(const SpeakerTurn& t);
    void clear();

    // Query API (any thread)
    int dominantSpeakerAt(uint64_t ms) const;          // argmax, or -1
    int activeSpeakerCountAt(uint64_t ms) const;       // channels over threshold
    std::vector<SpeakerTurn> turns() const;
    std::vector<ActivityFrame> framesSince(uint64_t ms) const;

private:
    struct ChannelOpen {
        bool active = false;
        uint64_t start = 0;
        float acc = 0.f;
        uint64_t n = 0;
    };

    // Incrementally converts prob frames in [materialized_up_to_, up_to)
    // into SpeakerTurns; open channels persist between calls.
    void materializeTurns(uint64_t up_to_ms);
    // The frame covering instant ms, or nullptr.
    const ActivityFrame* frameAt(uint64_t ms) const;

    Options opts_;
    mutable std::mutex m_;
    std::deque<ActivityFrame> frames_;
    std::vector<SpeakerTurn> turns_;
    std::array<ChannelOpen, kMaxSpeakers> open_{};
    uint64_t materialized_up_to_ = 0;
};

} // namespace ss
