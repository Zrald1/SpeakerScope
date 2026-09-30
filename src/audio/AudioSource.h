#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include "core/Types.h"

namespace ss {

class AudioRingBuffer;

// Any producer of 16 kHz mono s16 PCM pushed into an AudioRingBuffer.
class AudioSource {
public:
    virtual ~AudioSource() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual std::string describe() const = 0;
};

// factory: Microphone | Loopback | WavFile(path)
std::unique_ptr<AudioSource> makeAudioSource(AudioSourceType type,
                                             AudioRingBuffer& sink,
                                             const std::string& wav_path = "");

} // namespace ss
