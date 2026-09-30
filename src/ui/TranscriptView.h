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

private:
    int filter_ = -1; // -1 = everyone, else channel index

    static std::string joinForCopy(
        const std::vector<AttributedSegment>& segs,
        const SpeakerRegistry& registry, int filter);
};

} // namespace ss
