#pragma once

#include <cstdint>
#include "diar/SpeakerTimeline.h"

namespace ss {

// Helper queries for crosstalk display. Used by SpeakerAttributor and the
// SpeakerLanes widget.
inline int overlappingSpeakersAt(const SpeakerTimeline& tl, uint64_t ms) {
    return tl.activeSpeakerCountAt(ms);
}

} // namespace ss
