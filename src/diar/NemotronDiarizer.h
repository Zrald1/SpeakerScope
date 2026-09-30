#pragma once

#include <string>
#include "diar/DiarizationEngine.h"

namespace ss {

// On-device streaming diarization via NVIDIA NeMo-Speech.cpp (ggml/GGUF).
// Nemotron-3-Diarization: 100M params, 8 speakers, 10 ms output frames,
// AOSC + FIFO streaming state.
//
// Status: M4 stub — interface is final; the ggml session binding lands with
// the runtime integration. Until then isReady() reports whether the model
// file exists so the UI can show "diarization unavailable" vs "live".
class NemotronDiarizer : public DiarizationEngine {
public:
    explicit NemotronDiarizer(std::string model_path);
    ~NemotronDiarizer() override;

    void pushAudio(const int16_t* samples, size_t count,
                   uint64_t first_sample_ms) override;
    void setActivityCallback(
        std::function<void(const ActivityFrame&)> cb) override;
    bool isReady() const override { return ready_; }

    const std::string& statusLine() const { return status_; }

private:
    std::string model_path_;
    std::string status_;
    bool ready_ = false;
    std::function<void(const ActivityFrame&)> cb_;
};

} // namespace ss
