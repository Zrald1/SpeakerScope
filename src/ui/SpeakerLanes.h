#pragma once

#include <array>
#include <cstdint>
#include "core/Types.h"

namespace ss {

class SpeakerTimeline;
class SpeakerRegistry;

// Per-person lane rows. Each row = [editable name field][activity ribbon].
// Colored bars mark that person's speech over the trailing window; stacked
// simultaneous bars make overlaps obvious. Click a name to rename the person.
class SpeakerLanes {
public:
    // window_ms: trailing history shown (default 30 s).
    // Returns the clicked channel index, or -1 (for transcript filtering).
    void render(const SpeakerTimeline& timeline, SpeakerRegistry& registry,
                uint64_t now_ms, uint64_t window_ms = 30'000);

private:
    // Per-channel editable name buffers synced lazily from the registry.
    std::array<std::array<char, 64>, kMaxSpeakers> name_buf_{};
    std::array<bool, kMaxSpeakers> buf_init_{};
};

} // namespace ss
