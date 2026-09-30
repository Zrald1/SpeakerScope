#pragma once

#include <cstdint>
#include <vector>

namespace ss {

// Anything that maps a PCM segment to a fixed-size speaker embedding.
// Implementations: TitaNetEmbedder (GGUF, on-device), OnnxEmbedder
// (WeSpeaker ResNet34 via onnxruntime), FakeEmbedder (tests).
class IEmbedder {
public:
    virtual ~IEmbedder() = default;

    // samples: 16 kHz mono s16. Returns an L2-normalized embedding.
    virtual std::vector<float> embed(const int16_t* samples,
                                     size_t count) = 0;
    virtual size_t dimension() const = 0;
    virtual bool isReady() const = 0;
};

} // namespace ss
