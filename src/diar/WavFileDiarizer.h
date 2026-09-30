#pragma once

#include <vector>
#include "diar/DiarizationEngine.h"

namespace ss {

// Test double + demo aid: replays a known SpeakerTurn list as ActivityFrames
// driven by the timestamps of pushed audio. Lets the whole fusion + UI path
// run and be tested without the model.
class WavFileDiarizer : public DiarizationEngine {
public:
    // gold: ground-truth turns in stream-time ms (e.g., parsed RTTM).
    explicit WavFileDiarizer(std::vector<SpeakerTurn> gold);

    void pushAudio(const int16_t* samples, size_t count,
                   uint64_t first_sample_ms) override;
    void setActivityCallback(
        std::function<void(const ActivityFrame&)> cb) override;

private:
    std::vector<SpeakerTurn> gold_;
    std::function<void(const ActivityFrame&)> cb_;
    uint64_t emitted_up_to_ms_ = 0;
};

} // namespace ss
