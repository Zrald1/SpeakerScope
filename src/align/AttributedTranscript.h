#pragma once

#include <vector>
#include "align/SpeakerAttributor.h"
#include "core/Types.h"
#include "diar/SpeakerTimeline.h"
#include "stt/TranscriptBuffer.h"

namespace ss {

// Read model for the UI: combines TranscriptBuffer (words per turn) with
// SpeakerTimeline into a flat, ordered list of AttributedSegments.
class AttributedTranscript {
public:
    AttributedTranscript(const TranscriptBuffer& tb,
                         const SpeakerTimeline& tl)
        : tb_(tb), tl_(tl) {}

    // Recomputes the segment list; call once per UI frame.
    std::vector<AttributedSegment> snapshot() const;

    // Plain-text rendering for export ([Speaker N] text).
    std::string renderText(const class SpeakerRegistry& reg) const;

private:
    const TranscriptBuffer& tb_;
    const SpeakerTimeline& tl_;
    SpeakerAttributor attributor_;
};

} // namespace ss
