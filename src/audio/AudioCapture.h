#pragma once

#include <atomic>
#include "audio/AudioSource.h"

namespace ss {

// miniaudio-backed capture device. With loopback=true it attaches to the
// WASAPI render endpoint and captures system audio (meeting apps, browser,
// media players) instead of the microphone.
class AudioCapture : public AudioSource {
public:
    struct Impl; // defined in .cpp (keeps miniaudio.h out of this header)

    AudioCapture(AudioRingBuffer& sink, bool loopback);
    ~AudioCapture() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override { return running_.load(); }
    std::string describe() const override {
        return loopback_ ? "System loopback (WASAPI)" : "Microphone";
    }

private:
    Impl* impl_ = nullptr;

    AudioRingBuffer& sink_;
    bool loopback_;
    std::atomic<bool> running_{false};
};

} // namespace ss
