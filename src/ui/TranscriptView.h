#pragma once

#include <vector>
#include "core/Types.h"

namespace ss {

class SpeakerRegistry;

// Scrolling, speaker-colored transcript pane with a toolbar:
//   "Show: [Everyone | Person 1 | ...]" filter + "Copy" (clipboard) button.
class TranscriptView {
public:
    void render(const std::vector<AttributedSegment>& segments,
                SpeakerRegistry& registry);

    // "[Name] text\n" per segment, optionally filtered to one channel.
    static std::string joinForCopy(
        const std::vector<AttributedSegment>& segs,
        const SpeakerRegistry& registry, int filter);

private:
    int filter_ = -1; // -1 = everyone, else channel index
};

} // namespace ss
