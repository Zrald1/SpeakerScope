#include "audio/WavFileSource.h"

#include <miniaudio.h>

#include <chrono>
#include "audio/AudioRingBuffer.h"
#include "audio/Resampler.h"
#include "core/Clock.h"
#include "core/Logger.h"

namespace ss {

WavFileSource::WavFileSource(AudioRingBuffer& sink, std::string path)
    : sink_(sink), path_(std::move(path)) {}

WavFileSource::~WavFileSource() { stop(); }

bool WavFileSource::start() {
    if (running_.load()) return true;
    sink_.clear();
    stop_.store(false);
    running_.store(true);
    thread_ = std::thread(&WavFileSource::run, this);
    return true;
}

void WavFileSource::stop() {
    stop_.store(true);
    // Join even when running_ is already false — a finished thread is still
    // joinable, and destroying it would call std::terminate.
    if (thread_.joinable()) thread_.join();
    running_.store(false);
}

void WavFileSource::run() {
    ma_decoder_config cfg =
        ma_decoder_config_init(ma_format_s16, 0, 0); // keep file's channels/rate
    ma_decoder dec;
    if (ma_decoder_init_file(path_.c_str(), &cfg, &dec) != MA_SUCCESS) {
        log()->error("WavFileSource: cannot decode {}", path_);
        running_.store(false);
        return;
    }

    ma_uint32 src_rate = 0, src_ch = 0;
    ma_decoder_get_data_format(&dec, nullptr, &src_ch, &src_rate, nullptr, 0);
    Resampler resampler(src_rate, src_ch);
    log()->info("WavFileSource: {} ({} Hz, {} ch)", path_, src_rate, src_ch);

    constexpr size_t kReadFrames = 4096;
    std::vector<int16_t> in(kReadFrames * src_ch);
    const auto start_time = std::chrono::steady_clock::now();
    uint64_t emitted_ms = 0;

    while (!stop_.load()) {
        ma_uint64 got = 0;
        ma_result rr =
            ma_decoder_read_pcm_frames(&dec, in.data(), kReadFrames, &got);
        if (rr != MA_SUCCESS || got == 0) {
            log()->info("WavFileSource: read ended (result={}, frames={})",
                        static_cast<int>(rr), static_cast<uint64_t>(got));
            break; // EOF
        }
        auto out = resampler.process(in.data(), got);
        if (!out.empty()) {
            // Pace to real time so the streaming pipeline sees live-like input.
            emitted_ms += samplesToMs(out.size());
            sink_.push(out.data(), out.size());
            std::this_thread::sleep_until(
                start_time + std::chrono::milliseconds(emitted_ms));
        }
    }

    ma_decoder_uninit(&dec);
    log()->info("WavFileSource: finished {} ({} ms)", path_, emitted_ms);
    running_.store(false);
}

} // namespace ss
