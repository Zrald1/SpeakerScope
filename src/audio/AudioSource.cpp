#include "audio/AudioSource.h"

#include "audio/AudioCapture.h"
#include "audio/AudioRingBuffer.h"
#include "audio/WavFileSource.h"

namespace ss {

std::unique_ptr<AudioSource> makeAudioSource(AudioSourceType type,
                                             AudioRingBuffer& sink,
                                             const std::string& wav_path) {
    switch (type) {
        case AudioSourceType::Loopback:
            return std::make_unique<AudioCapture>(sink, true);
        case AudioSourceType::WavFile:
            return std::make_unique<WavFileSource>(sink, wav_path);
        case AudioSourceType::Microphone:
        default:
            return std::make_unique<AudioCapture>(sink, false);
    }
}

} // namespace ss
