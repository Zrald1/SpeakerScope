#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include "audio/AudioRingBuffer.h"
#include "audio/AudioSource.h"
#include "core/Clock.h"
#include "core/Types.h"
#include "diar/DiarizationEngine.h"
#include "diar/SpeakerRegistry.h"
#include "diar/SpeakerTimeline.h"
#include "stt/AssemblyAIClient.h"
#include "stt/TranscriptBuffer.h"

namespace ss {

struct Config;

// Owns one capture->tee->{STT, diar}->fusion session. UI drives it.
class SessionController {
public:
    SessionController();
    ~SessionController();

    bool start(const Config& cfg, AudioSourceType src,
               const std::string& wav_path = "");
    void stop();

    SessionState state() const { return state_.load(); }
    std::string statusLine() const;

    // Read models for the UI.
    const TranscriptBuffer& transcript() const { return transcript_; }
    const SpeakerTimeline& timeline() const { return timeline_; }
    SpeakerRegistry& registry() { return registry_; }
    uint64_t elapsedMs() const { return samplesToMs(samples_sent_.load()); }

    SessionController(const SessionController&) = delete;
    SessionController& operator=(const SessionController&) = delete;

private:
    void senderLoop();
    void onServerEvent(const ServerEvent& ev);

    AudioRingBuffer ring_;
    std::unique_ptr<AudioSource> source_;
    AssemblyAIClient stt_;
    std::unique_ptr<DiarizationEngine> diar_;

    TranscriptBuffer transcript_;
    SpeakerTimeline timeline_;
    SpeakerRegistry registry_;

    std::thread sender_thread_;
    std::atomic<bool> stop_{false};
    std::atomic<uint64_t> samples_sent_{0};
    std::atomic<SessionState> state_{SessionState::Idle};
    std::string error_;
};

} // namespace ss
