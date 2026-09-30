#pragma once

#include <string>
#include <string_view>
#include "core/Types.h"

namespace ss {

// AssemblyAI Streaming v3 server -> client message types.
// See docs: https://www.assemblyai.com/docs/streaming/message-sequence
enum class ServerMsgType {
    Begin,
    Turn,
    SpeechStarted,
    SpeakerRevision,
    Heartbeat,
    Termination,
    Error,
    Unknown
};

struct ServerEvent {
    ServerMsgType type = ServerMsgType::Unknown;

    // Begin
    std::string session_id;
    uint64_t expires_at = 0;

    // Turn
    Turn turn;
    float confidence = 0.0f;

    // SpeechStarted
    uint64_t audio_ms = 0;

    // Error / Termination
    std::string detail;
};

// Parses one JSON text frame into a ServerEvent. Never throws on bad input.
ServerEvent parseServerMessage(std::string_view json_text);

} // namespace ss
