#include "app/SessionController.h"

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

    stt_.connect(cfg, [this](const ServerEvent& ev) { onServerEvent(ev); });
    source_->start();

    sender_thread_ = std::thread(&SessionController::senderLoop, this);
    return true;
}

void SessionController::stop() {
    if (state_.load() == SessionState::Idle) return;
    stop_.store(true);
    if (sender_thread_.joinable()) sender_thread_.join();
    if (source_) { source_->stop(); source_.reset(); }
    stt_.terminate();
    diar_.reset();
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
        const uint64_t first_ms = samplesToMs(samples_sent_.load());
        stt_.sendAudio(chunk.data(), got);
        if (diar_) diar_->pushAudio(chunk.data(), got, first_ms);
        samples_sent_.fetch_add(got);
    }
}

} // namespace ss
