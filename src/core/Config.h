#pragma once

#include <string>
#include "core/Types.h"

namespace ss {

struct Config {
    std::string api_key;
    std::string ws_endpoint = "wss://streaming.assemblyai.com/v3/ws";
    std::string speech_model = "u3-rt-pro";
    int sample_rate = kSampleRate;
    int min_turn_silence_ms = 200;
    int max_turn_silence_ms = 1280;
    std::string diar_model_path = "models/nemotron-3-diarization-bf16.gguf";
    std::string hf_token;

    // Loads .env next to the executable/CWD, then real env vars override.
    static Config load();
    bool valid() const { return !api_key.empty(); }
};

} // namespace ss
