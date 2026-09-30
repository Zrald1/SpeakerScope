#pragma once

#include <atomic>
#include <string>
#include <thread>
#include "audio/AudioSource.h"

namespace ss {

// Streams a .wav file into the ring buffer at real-time pace.
// Indispensable for repeatable demos: point it at a recorded multi-speaker clip.
class WavFileSource : public AudioSource {
public:
    WavFileSource(AudioRingBuffer& sink, std::string path);
    ~WavFileSource() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override { return running_.load(); }
    std::string describe() const override { return "WAV file: " + path_; }

private:
    void run();

    AudioRingBuffer& sink_;
    std::string path_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_{false};
};

} // namespace ss
