#include "diar/NemotronDiarizer.h"

#include <filesystem>
#include "core/Logger.h"

namespace ss {

NemotronDiarizer::NemotronDiarizer(std::string model_path)
    : model_path_(std::move(model_path)) {
    ready_ = std::filesystem::exists(model_path_);
    status_ = ready_
                  ? "model found: " + model_path_
                  : "model missing — run scripts/download_models.ps1";
    if (ready_)
        log()->info("Diarization: {}", status_);
    else
        log()->warn("Diarization: {}", status_);
}

NemotronDiarizer::~NemotronDiarizer() = default;

void NemotronDiarizer::pushAudio(const int16_t* /*samples*/, size_t /*count*/,
                                 uint64_t /*first_sample_ms*/) {
    // TODO(M4): feed into NeMo-Speech.cpp streaming session
    // (nemotron_3_diar family, low-latency profile, AOSC + FIFO state),
    // decode [T,8] prob frames at 10 ms stride, emit via cb_.
}

void NemotronDiarizer::setActivityCallback(
    std::function<void(const ActivityFrame&)> cb) {
    cb_ = std::move(cb);
}

} // namespace ss
