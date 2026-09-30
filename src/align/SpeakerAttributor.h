#pragma once

#include <vector>
#include "core/Types.h"
#include "diar/SpeakerTimeline.h"

namespace ss {

// Fuses ASR words (AssemblyAI Turn.word timestamps) with the diarization
// speaker timeline into AttributedSegments.
//
// Algorithm:
//   for each word, s* = argmax speaker prob at the word's midpoint
//   hysteresis: a speaker switch must hold for >= min_switch_ms to stick
//   grouping: consecutive words with same s* merge into one segment
//   overlap: segment flagged when >=2 channels active at its midpoint
class SpeakerAttributor {
public:
    struct Options {
        uint64_t min_switch_ms = 200;
    };

    SpeakerAttributor() = default;
    explicit SpeakerAttributor(Options opts) : opts_(opts) {}

    std::vector<AttributedSegment> attribute(const std::vector<Word>& words,
                                             bool partial,
                                             const SpeakerTimeline& tl) const;

private:
    Options opts_;
};

} // namespace ss
