#include "audio/Resampler.h"

#include <miniaudio.h>

#include "core/Types.h"

namespace ss {

struct Resampler::Impl {
    ma_resampler resampler{};
    uint32_t src_channels;
};

Resampler::Resampler(uint32_t src_rate, uint32_t src_channels) {
    impl_ = new Impl();
    impl_->src_channels = src_channels;
    ma_resampler_config cfg =
        ma_resampler_config_init(ma_format_s16, src_channels, src_rate,
                                 kSampleRate, ma_resample_algorithm_linear);
    ma_resampler_init(&cfg, nullptr, &impl_->resampler);
}

Resampler::~Resampler() {
    ma_resampler_uninit(&impl_->resampler, nullptr);
    delete impl_;
}

std::vector<int16_t> Resampler::process(const int16_t* data, size_t frames) {
    ma_uint64 in_count = frames;
    // Worst-case output is roughly frames * (16000/src) + margin; grow buffer.
    std::vector<int16_t> out(frames * 2 + 4096);
    ma_uint64 out_frames = out.size() / kChannels;
    ma_result r = ma_resampler_process_pcm_frames(
        &impl_->resampler, data, &in_count, out.data(), &out_frames);
    if (r != MA_SUCCESS) return {};
    out.resize(static_cast<size_t>(out_frames) * kChannels);
    return out;
}

} // namespace ss
