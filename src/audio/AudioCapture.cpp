#include "audio/AudioCapture.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "audio/AudioRingBuffer.h"
#include "core/Logger.h"

namespace ss {

struct AudioCapture::Impl {
    ma_device device{};
    bool initialized = false;
    AudioRingBuffer* sink = nullptr;
};

AudioCapture::AudioCapture(AudioRingBuffer& sink, bool loopback)
    : sink_(sink), loopback_(loopback) {}

AudioCapture::~AudioCapture() { stop(); }

static void dataCallback(ma_device* device, void* /*output*/, const void* input,
                         ma_uint32 frameCount) {
    auto* impl = static_cast<AudioCapture::Impl*>(device->pUserData);
    if (impl && impl->sink && input) {
        impl->sink->push(static_cast<const int16_t*>(input),
                         static_cast<size_t>(frameCount));
    }
}

bool AudioCapture::start() {
    if (running_.load()) return true;
    sink_.clear();

    impl_ = new Impl();
    impl_->sink = &sink_;

    ma_device_config cfg =
        ma_device_config_init(loopback_ ? ma_device_type_loopback
                                        : ma_device_type_capture);
    // Device performs format/rate conversion for us: we always get s16 mono 16k.
    cfg.capture.format = ma_format_s16;
    cfg.capture.channels = kChannels;
    cfg.sampleRate = kSampleRate;
    cfg.dataCallback = dataCallback;
    cfg.pUserData = impl_;
    cfg.periodSizeInMilliseconds = 20;

    if (ma_device_init(nullptr, &cfg, &impl_->device) != MA_SUCCESS) {
        log()->error("miniaudio device init failed ({})", describe());
        delete impl_;
        impl_ = nullptr;
        return false;
    }
    impl_->initialized = true;

    if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        log()->error("miniaudio device start failed ({})", describe());
        ma_device_uninit(&impl_->device);
        delete impl_;
        impl_ = nullptr;
        return false;
    }

    running_.store(true);
    log()->info("Audio capture started: {}", describe());
    return true;
}

void AudioCapture::stop() {
    if (!running_.load()) return;
    running_.store(false);
    if (impl_ && impl_->initialized) {
        ma_device_stop(&impl_->device);
        ma_device_uninit(&impl_->device);
    }
    delete impl_;
    impl_ = nullptr;
    log()->info("Audio capture stopped");
}

} // namespace ss
