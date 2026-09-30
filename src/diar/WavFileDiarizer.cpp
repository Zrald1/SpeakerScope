#include "diar/WavFileDiarizer.h"

#include "core/Clock.h"

namespace ss {

WavFileDiarizer::WavFileDiarizer(std::vector<SpeakerTurn> gold)
    : gold_(std::move(gold)) {}

void WavFileDiarizer::pushAudio(const int16_t* /*samples*/, size_t count,
                                uint64_t first_sample_ms) {
    const uint64_t end_ms = first_sample_ms + samplesToMs(count);
    // Emit 10 ms frames covering the pushed range.
    for (uint64_t t = emitted_up_to_ms_; t < end_ms; t += 10) {
        ActivityFrame f;
        f.start_ms = t;
        for (const auto& turn : gold_) {
            if (turn.speaker >= 0 && turn.speaker < kMaxSpeakers &&
                t >= turn.start_ms && t < turn.end_ms) {
                f.probs[turn.speaker] = turn.confidence > 0 ? turn.confidence
                                                          : 0.95f;
            }
        }
        if (cb_) cb_(f);
    }
    emitted_up_to_ms_ = end_ms;
}

void WavFileDiarizer::setActivityCallback(
    std::function<void(const ActivityFrame&)> cb) {
    cb_ = std::move(cb);
}

} // namespace ss
