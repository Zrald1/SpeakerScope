#pragma once

#include <string>
#include "diar/DiarizationEngine.h"

// audio.cpp C ABI handle (audiocpp.h) — declared here so the header needs
// no include; SS_HAS_AUDIOCPP guards the member that uses it.
struct audiocpp_event;

namespace ss {

// On-device streaming diarization via audio.cpp (ggml/GGUF C ABI).
// Nemotron-3-Diarization: 100M params, 8 arrival-ordered channels,
// AOSC + FIFO streaming state, ~1 s latency on the "low" profile.
//
// The runtime reports *decoded closed turns* (not prob frames), so this
// engine drives SpeakerTimeline::addTurn via setTurnCallback. When the
// audio.cpp shared library is not linked (SS_HAS_AUDIOCPP undefined) or
// the model can't load, it degrades to a no-op stub and isReady()==false.
class NemotronDiarizer : public DiarizationEngine {
public:
    explicit NemotronDiarizer(std::string model_path);
    ~NemotronDiarizer() override;

    void pushAudio(const int16_t* samples, size_t count,
                   uint64_t first_sample_ms) override;
    void finish() override;
    void setActivityCallback(
        std::function<void(const ActivityFrame&)> cb) override;
    void setTurnCallback(
        std::function<void(const SpeakerTurn&)> cb) override;
    bool isReady() const override { return ready_; }

    const std::string& statusLine() const { return status_; }

private:
    struct Impl;
    Impl* impl_;

#ifdef SS_HAS_AUDIOCPP
    void emitEventTurns(const struct audiocpp_event* ev);
#endif

    std::string model_path_;
    std::string status_;
    bool ready_ = false;
    std::function<void(const ActivityFrame&)> cb_;
    std::function<void(const SpeakerTurn&)> turn_cb_;
};

} // namespace ss
