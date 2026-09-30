#include "stt/AssemblyAIClient.h"

#include <ixwebsocket/IXWebSocket.h>

#include <nlohmann/json.hpp>

#include "core/Config.h"
#include "core/Logger.h"
#include "core/Types.h"

namespace ss {

namespace {

std::string buildUrl(const Config& cfg) {
    std::string url = cfg.ws_endpoint;
    url += "?sample_rate=" + std::to_string(cfg.sample_rate);
    url += "&encoding=pcm_s16le";
    url += "&speech_model=" + cfg.speech_model;
    url += "&format_turns=true";
    url += "&min_turn_silence=" + std::to_string(cfg.min_turn_silence_ms);
    url += "&max_turn_silence=" + std::to_string(cfg.max_turn_silence_ms);
    return url;
}

} // namespace

AssemblyAIClient::AssemblyAIClient() = default;
AssemblyAIClient::~AssemblyAIClient() { terminate(); }

bool AssemblyAIClient::connect(const Config& cfg, EventCallback cb) {
    if (cfg.api_key.empty()) {
        last_error_ = "ASSEMBLYAI_API_KEY not set";
        return false;
    }
    cb_ = std::move(cb);
    ws_ = std::make_unique<ix::WebSocket>();
    ws_->setUrl(buildUrl(cfg));
    ws_->setExtraHeaders({{"Authorization", cfg.api_key}});
    ws_->setPingInterval(20);
    ws_->disablePerMessageDeflate(); // binary PCM doesn't benefit

    ws_->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        switch (msg->type) {
            case ix::WebSocketMessageType::Open:
                open_.store(true);
                log()->info("STT: websocket open");
                break;
            case ix::WebSocketMessageType::Close:
                open_.store(false);
                log()->info("STT: websocket closed ({} {})", msg->closeInfo.code,
                            msg->closeInfo.reason);
                break;
            case ix::WebSocketMessageType::Error:
                last_error_ = msg->errorInfo.reason;
                log()->error("STT: websocket error: {}", last_error_);
                break;
            case ix::WebSocketMessageType::Message:
                if (cb_) cb_(parseServerMessage(msg->str));
                break;
            default:
                break;
        }
    });

    ws_->start();
    log()->info("STT: connecting to {}", cfg.ws_endpoint);
    return true;
}

void AssemblyAIClient::sendAudio(const int16_t* samples, size_t count) {
    if (!open_.load() || !samples || count == 0) return;
    const char* bytes = reinterpret_cast<const char*>(samples);
    ws_->sendBinary(std::string(bytes, bytes + count * kBytesPerSample));
    bytes_sent_.fetch_add(count * kBytesPerSample);
}

void AssemblyAIClient::terminate() {
    if (!ws_) return;
    if (open_.load()) {
        // Billing runs while the socket is open — always terminate cleanly.
        ws_->sendText(R"({"type":"Terminate"})");
    }
    ws_->stop();
    open_.store(false);
    log()->info("STT: terminated ({} bytes of audio sent)", bytes_sent_.load());
}

} // namespace ss
