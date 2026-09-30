#include "app/SessionController.h"

#include <algorithm>
#include <chrono>
#include "core/Clock.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "diar/NemotronDiarizer.h"

namespace ss {

SessionController::SessionController() = default;
SessionController::~SessionController() { stop(); }

bool SessionController::start(const Config& cfg, AudioSourceType src,
                              const std::string& wav_path,
                              std::unique_ptr<DiarizationEngine> diar) {
    if (state_.load() != SessionState::Idle) return false;
    if (!cfg.valid()) {
        error_ = "ASSEMBLYAI_API_KEY missing (put it in .env)";
        state_.store(SessionState::Error);
        return false;
    }

    state_.store(SessionState::Connecting);
    error_.clear();
    transcript_.clear();
    timeline_.clear();
    samples_sent_.store(0);
    stop_.store(false);

    source_ = makeAudioSource(src, ring_, wav_path);
    diar_ = diar ? std::move(diar)
                 : std::unique_ptr<DiarizationEngine>(
                       new NemotronDiarizer(cfg.diar_model_path));
    diar_->setActivityCallback([this](const ActivityFrame& f) {
        timeline_.addFrame(f);
    });
    diar_->setTurnCallback([this](const SpeakerTurn& t) {
        timeline_.addTurn(t);
    });

    stt_.connect(cfg, [this](const ServerEvent& ev) { onServerEvent(ev); });
    source_->start();

    sender_thread_ = std::thread(&SessionController::senderLoop, this);
    if (diar_ && diar_->isReady())
        diar_thread_ = std::thread(&SessionController::diarLoop, this);
    return true;
}

void SessionController::stop() {
    if (state_.load() == SessionState::Idle) return;
    stop_.store(true);
    if (sender_thread_.joinable()) sender_thread_.join();
    // Close the STT socket before draining the diar backlog — AssemblyAI
    // bills while the session is open, and the drain can take a while.
    stt_.terminate();
    if (diar_thread_.joinable()) diar_thread_.join();
    if (source_) { source_->stop(); source_.reset(); }
    if (diar_) { diar_->finish(); diar_.reset(); }
    state_.store(SessionState::Idle);
    log()->info("Session stopped");
}

std::string SessionController::statusLine() const {
    switch (state_.load()) {
        case SessionState::Idle: return "idle";
        case SessionState::Connecting: return "connecting…";
        case SessionState::Live: return "live";
        case SessionState::Stopping: return "stopping…";
        case SessionState::Error: return "error: " + error_;
    }
    return "?";
}

void SessionController::onServerEvent(const ServerEvent& ev) {
    switch (ev.type) {
        case ServerMsgType::Begin:
            log()->info("STT session {} (expires {})", ev.session_id,
                        ev.expires_at);
            state_.store(SessionState::Live);
            break;
        case ServerMsgType::Turn:
            transcript_.onTurn(ev.turn);
            break;
        case ServerMsgType::Error:
            error_ = ev.detail;
            state_.store(SessionState::Error);
            log()->error("STT server error: {}", ev.detail);
            break;
        case ServerMsgType::Termination:
            log()->info("STT session terminated");
            break;
        default:
            break;
    }
}

void SessionController::senderLoop() {
    std::vector<int16_t> chunk(kChunkSamples);
    while (!stop_.load()) {
        // AssemblyAI requires 50–1000 ms per binary frame — only send full
        // 50 ms chunks; partial tails wait for more audio.
        if (ring_.available() < kChunkSamples) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        size_t got = ring_.pop(chunk.data(), kChunkSamples);
        stt_.sendAudio(chunk.data(), got);
        // Tee into the diarization queue (non-blocking; truncates when full).
        if (diar_ && diar_thread_.joinable())
            ring_diar_.push(chunk.data(), got);
        samples_sent_.fetch_add(got);
    }
}

void SessionController::diarLoop() {
    std::vector<int16_t> chunk(kSampleRate); // up to 1 s per push
    // Drain on exit too — stop() must not strand queued audio that the model
    // hasn't seen, or tail turns never materialize.
    while (true) {
        const size_t avail = ring_diar_.available();
        if (avail == 0 && stop_.load()) break;
        if (avail < kChunkSamples && !stop_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        if (avail == 0) break;
        size_t got = ring_diar_.pop(chunk.data(), std::min(avail, chunk.size()));
        diar_->pushAudio(chunk.data(), got, samplesToMs(diar_samples_.load()));
        diar_samples_.fetch_add(got);
    }
}

} // namespace ss
