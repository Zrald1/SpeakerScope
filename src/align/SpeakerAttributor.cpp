#include "align/SpeakerAttributor.h"

namespace ss {

std::vector<AttributedSegment>
SpeakerAttributor::attribute(const std::vector<Word>& words, bool partial,
                             const SpeakerTimeline& tl) const {
    std::vector<AttributedSegment> out;
    if (words.empty()) return out;

    // First pass: raw speaker + overlap flag per word.
    std::vector<int> raw(words.size(), -1);
    std::vector<bool> ovl(words.size(), false);
    for (size_t i = 0; i < words.size(); ++i) {
        uint64_t mid =
            (static_cast<uint64_t>(words[i].start_ms) + words[i].end_ms) / 2;
        raw[i] = tl.dominantSpeakerAt(mid);
        ovl[i] = tl.activeSpeakerCountAt(mid) >= 2;
    }

    // Second pass: hysteresis — suppress switches that revert within
    // min_switch_ms (single-word flicker gets folded into neighbors).
    for (size_t i = 1; i + 1 < raw.size(); ++i) {
        if (raw[i] != raw[i - 1] && raw[i] != raw[i + 1]) {
            uint64_t span = words[i + 1].start_ms - words[i].start_ms;
            if (span < opts_.min_switch_ms) raw[i] = raw[i - 1];
        }
    }

    // Third pass: group runs.
    for (size_t i = 0; i < words.size(); ++i) {
        const Word& w = words[i];
        if (out.empty() || out.back().speaker != raw[i] ||
            out.back().partial != partial) {
            AttributedSegment seg;
            seg.speaker = raw[i];
            seg.start_ms = w.start_ms;
            seg.partial = partial;
            out.push_back(seg);
        }
        auto& seg = out.back();
        if (!seg.text.empty()) seg.text += ' ';
        seg.text += w.text;
        seg.end_ms = w.end_ms;
        seg.overlap = seg.overlap || ovl[i];
    }
    return out;
}

} // namespace ss
