#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include "core/Types.h"

namespace ss {

// Per-frame speaker-activity probabilities: 8 channels, 10 ms stride.
struct ActivityFrame {
    uint64_t start_ms = 0;
    std::array<float, kMaxSpeakers> probs{};
};

// Interface for any engine that answers "who is active when".
// Implementations: NemotronDiarizer (on-device GGUF, M4),
// WavFileDiarizer (test double replaying gold labels).
class DiarizationEngine {
public:
    virtual ~DiarizationEngine() = default;

    // Feed raw 16 kHz mono s16 PCM. first_sample_ms = stream-time of data[0].
    virtual void pushAudio(const int16_t* samples, size_t count,
                           uint64_t first_sample_ms) = 0;

    // Emitted as the engine emits new activity frames (worker thread!).
    virtual void setActivityCallback(
        std::function<void(const ActivityFrame&)>) = 0;

    virtual bool isReady() const { return true; }
};

} // namespace ss
