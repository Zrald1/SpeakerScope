#pragma once

#include <cstdint>
#include <vector>

namespace ss {

// Thin wrapper over miniaudio's resampler for the wav-file path and tests.
// Live capture doesn't need it (the device hands us 16 kHz s16 directly).
class Resampler {
public:
    Resampler(uint32_t src_rate, uint32_t src_channels);
    ~Resampler();

    // s16 interleaved input -> s16 mono 16 kHz output.
    std::vector<int16_t> process(const int16_t* data, size_t frames);

    Resampler(const Resampler&) = delete;
    Resampler& operator=(const Resampler&) = delete;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace ss
