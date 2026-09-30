#include "diar/NemotronDiarizer.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <thread>
#include <set>
#include <tuple>
#include <vector>
#include "core/Logger.h"

#ifdef SS_HAS_AUDIOCPP
#include "audiocpp.h"
#endif

namespace ss {

namespace {
constexpr const char* kSpeakerPrefix = "speaker_";

int parseSpeakerChannel(const char* id) {
    if (!id || std::strncmp(id, kSpeakerPrefix, std::strlen(kSpeakerPrefix)) != 0)
        return -1;
    return std::atoi(id + std::strlen(kSpeakerPrefix));
}
} // namespace

struct NemotronDiarizer::Impl {
#ifdef SS_HAS_AUDIOCPP
    audiocpp_registry* reg = nullptr;
    audiocpp_model* model = nullptr;
    audiocpp_session* session = nullptr;
#endif
    std::vector<float> fbuf;                 // s16 -> float scratch
    int64_t pushed_samples = 0;              // stream position (samples)
    bool finished = false;
    std::mutex m;
    std::set<std::tuple<int, int64_t, int64_t>> seen;  // turn dedup
};

NemotronDiarizer::NemotronDiarizer(std::string model_path)
    : model_path_(std::move(model_path)), impl_(new Impl) {
#ifdef SS_HAS_AUDIOCPP
    if (!std::filesystem::exists(model_path_)) {
        status_ = "model missing — run scripts/download_models.ps1";
        log()->warn("Diarization: {}", status_);
        return;
    }
    if (audiocpp_registry_create(nullptr, &impl_->reg) != AUDIOCPP_OK) {
        status_ = "audiocpp registry init failed";
        log()->error("Diarization: {}", status_);
        return;
    }
    audiocpp_model_config mc{};
    mc.family_hint = "nemotron_3_diar";
    if (audiocpp_model_load(impl_->reg, model_path_.c_str(), &mc, nullptr,
                            &impl_->model) != AUDIOCPP_OK) {
        status_ = "failed to load " + model_path_;
        log()->error("Diarization: {}", status_);
        return;
    }
    if (!audiocpp_model_supports(impl_->model, "diar", "streaming")) {
        status_ = "model does not support streaming diarization";
        log()->error("Diarization: {}", status_);
        return;
    }
    audiocpp_options* opts = audiocpp_options_create();
    // Low-latency streaming profile: shorter lookahead, ~1 s turnaround.
    audiocpp_options_set(opts, "nemotron_3_diar.latency_profile", "low");
    audiocpp_backend_config backend{};
    backend.backend = "cpu";
    backend.device = 0;
    const unsigned hw = std::thread::hardware_concurrency();
    backend.threads = hw > 0 ? static_cast<int>(hw) : 4;  // 0 is rejected
    auto st = audiocpp_session_create(impl_->model, "diar", "streaming",
                                      &backend, opts, &impl_->session);
    audiocpp_options_free(opts);
    if (st != AUDIOCPP_OK || !impl_->session) {
        status_ = "failed to create diar session";
        log()->error("Diarization: {}", status_);
        return;
    }
    // Request options: pad decoded turns ~300 ms each side so intra-sentence
    // silence dips don't split a speaker's coverage into [?] islands, and
    // drop ultra-short blips below 300 ms.
    audiocpp_request* req = audiocpp_request_create();
    audiocpp_request_set_option(req, "speaker_pad_frames", "30");
    audiocpp_request_set_option(req, "speaker_min_frames", "30");
    auto start_st = audiocpp_stream_start(impl_->session, req);
    audiocpp_request_free(req);
    if (start_st != AUDIOCPP_OK) {
        status_ = "failed to start diar stream";
        log()->error("Diarization: {}", status_);
        return;
    }
    ready_ = true;
    status_ = "nemotron-3-diarization live (audio.cpp, low latency)";
    log()->info("Diarization: {}", status_);
#else
    ready_ = std::filesystem::exists(model_path_);
    status_ = ready_
        ? "model found, but audio.cpp runtime not linked (stub)"
        : "model missing — run scripts/download_models.ps1";
    log()->warn("Diarization: {}", status_);
#endif
}

NemotronDiarizer::~NemotronDiarizer() {
#ifdef SS_HAS_AUDIOCPP
    if (impl_->session) {
        audiocpp_stream_reset(impl_->session);
        audiocpp_session_free(impl_->session);
    }
    if (impl_->model) audiocpp_model_free(impl_->model);
    if (impl_->reg) audiocpp_registry_free(impl_->reg);
#endif
    delete impl_;
}

#ifdef SS_HAS_AUDIOCPP
void NemotronDiarizer::emitEventTurns(const audiocpp_event* ev) {
    if (!ev) return;
    const audiocpp_result* r = audiocpp_event_as_result(ev);
    if (!r) return;
    const size_t n = audiocpp_result_speaker_turn_count(r);
    for (size_t i = 0; i < n; ++i) {
        int64_t s = 0, e = 0;
        const char* id = nullptr;
        float conf = 0.f;
        const char* text = nullptr;
        if (audiocpp_result_speaker_turn(r, i, &s, &e, &id, &conf, &text) !=
            AUDIOCPP_OK)
            continue;
        const int ch = parseSpeakerChannel(id);
        if (ch < 0 || ch >= kMaxSpeakers) continue;
        if (!impl_->seen.emplace(ch, s, e).second) continue;
        SpeakerTurn t;
        t.speaker = ch;
        t.start_ms = static_cast<uint64_t>(s) / (kSampleRate / 1000);
        t.end_ms = static_cast<uint64_t>(e) / (kSampleRate / 1000);
        t.confidence = conf;
        if (turn_cb_) turn_cb_(t);
    }
}
#endif

void NemotronDiarizer::pushAudio(const int16_t* samples, size_t count,
                                 uint64_t /*first_sample_ms*/) {
#ifdef SS_HAS_AUDIOCPP
    if (!ready_ || !impl_->session || impl_->finished) return;
    std::lock_guard lk(impl_->m);
    impl_->fbuf.resize(count);
    for (size_t i = 0; i < count; ++i)
        impl_->fbuf[i] = static_cast<float>(samples[i]) / 32768.0f;

    audiocpp_event* ev = nullptr;
    auto st = audiocpp_stream_push(impl_->session, impl_->fbuf.data(), count,
                                   kSampleRate, 1, impl_->pushed_samples, &ev);
    impl_->pushed_samples += static_cast<int64_t>(count);
    if (st != AUDIOCPP_OK) {
        log()->warn("Diarization: stream_push failed ({})", static_cast<int>(st));
        return;
    }
    emitEventTurns(ev);
    if (ev) audiocpp_event_free(ev);
    // Drain any events the family queued itself.
    while (audiocpp_stream_next_event(impl_->session, &ev) == AUDIOCPP_OK &&
           ev) {
        emitEventTurns(ev);
        audiocpp_event_free(ev);
    }
#else
    (void)samples; (void)count;
#endif
}

void NemotronDiarizer::finish() {
#ifdef SS_HAS_AUDIOCPP
    std::lock_guard lk(impl_->m);
    if (!impl_->session || impl_->finished) return;
    impl_->finished = true;
    audiocpp_result* r = nullptr;
    if (audiocpp_stream_finish(impl_->session, &r) != AUDIOCPP_OK || !r) return;
    const size_t n = audiocpp_result_speaker_turn_count(r);
    for (size_t i = 0; i < n; ++i) {
        int64_t s = 0, e = 0;
        const char* id = nullptr;
        float conf = 0.f;
        const char* text = nullptr;
        if (audiocpp_result_speaker_turn(r, i, &s, &e, &id, &conf, &text) !=
            AUDIOCPP_OK)
            continue;
        const int ch = parseSpeakerChannel(id);
        if (ch < 0 || ch >= kMaxSpeakers) continue;
        if (!impl_->seen.emplace(ch, s, e).second) continue;
        SpeakerTurn t;
        t.speaker = ch;
        t.start_ms = static_cast<uint64_t>(s) / (kSampleRate / 1000);
        t.end_ms = static_cast<uint64_t>(e) / (kSampleRate / 1000);
        t.confidence = conf;
        if (turn_cb_) turn_cb_(t);
    }
    audiocpp_result_free(r);
#endif
}

void NemotronDiarizer::setActivityCallback(
    std::function<void(const ActivityFrame&)> cb) {
    cb_ = std::move(cb);
}

void NemotronDiarizer::setTurnCallback(
    std::function<void(const SpeakerTurn&)> cb) {
    turn_cb_ = std::move(cb);
}

} // namespace ss
