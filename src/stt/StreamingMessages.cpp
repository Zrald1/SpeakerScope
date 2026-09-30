#include "stt/StreamingMessages.h"

#include <nlohmann/json.hpp>

#include "core/Logger.h"

namespace ss {

using nlohmann::json;

namespace {

ServerMsgType typeOf(const std::string& t) {
    if (t == "Begin") return ServerMsgType::Begin;
    if (t == "Turn") return ServerMsgType::Turn;
    if (t == "SpeechStarted") return ServerMsgType::SpeechStarted;
    if (t == "SpeakerRevision") return ServerMsgType::SpeakerRevision;
    if (t == "Heartbeat") return ServerMsgType::Heartbeat;
    if (t == "Termination") return ServerMsgType::Termination;
    if (t == "Error") return ServerMsgType::Error;
    return ServerMsgType::Unknown;
}

} // namespace

ServerEvent parseServerMessage(std::string_view json_text) {
    ServerEvent ev;
    json j = json::parse(json_text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object()) {
        log()->warn("STT: unparseable frame ({} bytes)", json_text.size());
        return ev;
    }

    ev.type = typeOf(j.value("type", std::string{}));

    switch (ev.type) {
        case ServerMsgType::Begin:
            ev.session_id = j.value("id", "");
            ev.expires_at = j.value("expires_at", uint64_t{0});
            break;

        case ServerMsgType::Turn: {
            ev.turn.turn_order = j.value("turn_order", uint32_t{0});
            ev.turn.transcript = j.value("transcript", "");
            ev.turn.end_of_turn = j.value("end_of_turn", false);
            ev.turn.end_of_turn_confidence =
                j.value("end_of_turn_confidence", 0.0);
            ev.confidence = j.value("confidence", 0.0f);
            if (auto it = j.find("words"); it != j.end() && it->is_array()) {
                for (const auto& w : *it) {
                    Word word;
                    word.text = w.value("text", "");
                    word.start_ms = w.value("start", uint32_t{0});
                    word.end_ms = w.value("end", uint32_t{0});
                    word.confidence = w.value("confidence", 0.0f);
                    word.word_is_final = w.value("word_is_final", false);
                    ev.turn.words.push_back(std::move(word));
                }
            }
            break;
        }

        case ServerMsgType::SpeechStarted:
            ev.audio_ms = j.value("audio_start", uint64_t{0});
            break;

        case ServerMsgType::Termination:
            ev.detail = j.value("audio_duration_seconds", 0.0) > 0
                            ? "session terminated"
                            : "terminated";
            break;

        case ServerMsgType::Error:
            ev.detail = j.value("error", j.dump());
            break;

        default:
            break;
    }
    return ev;
}

} // namespace ss
