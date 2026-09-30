#include "align/AttributedTranscript.h"

#include "diar/SpeakerRegistry.h"

namespace ss {

std::vector<AttributedSegment> AttributedTranscript::snapshot() const {
    std::vector<AttributedSegment> out;
    for (const auto& t : tb_.finalizedTurns()) {
        auto segs = attributor_.attribute(t.words, /*partial=*/false, tl_);
        out.insert(out.end(), segs.begin(), segs.end());
    }
    for (const auto& t : tb_.partialTurns()) {
        auto segs = attributor_.attribute(t.words, /*partial=*/true, tl_);
        out.insert(out.end(), segs.begin(), segs.end());
    }
    return out;
}

std::string
AttributedTranscript::renderText(const SpeakerRegistry& reg) const {
    std::string out;
    for (const auto& seg : snapshot()) {
        if (seg.speaker >= 0) {
            out += "[" + reg.name(seg.speaker) + "] ";
        }
        out += seg.text + "\n";
    }
    return out;
}

} // namespace ss
