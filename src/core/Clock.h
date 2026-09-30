#pragma once

#include <chrono>
#include <cstdint>
#include "core/Types.h"

namespace ss {

// All cross-stream alignment happens in the sample-index domain. Every PCM
// sample carries a monotonic index from capture start; ms = index / 16.

inline uint64_t samplesToMs(uint64_t samples) { return samples * 1000 / kSampleRate; }
inline uint64_t msToSamples(uint64_t ms) { return ms * kSampleRate / 1000; }

inline uint64_t nowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

} // namespace ss
