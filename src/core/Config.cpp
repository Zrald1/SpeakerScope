#include "core/Config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace ss {

namespace {

std::string trim(std::string s) {
    const char* ws = " \t\r\n";
    auto b = s.find_first_not_of(ws);
    auto e = s.find_last_not_of(ws);
    return b == std::string::npos ? "" : s.substr(b, e - b + 1);
}

std::unordered_map<std::string, std::string> parseEnvFile(const std::string& path) {
    std::unordered_map<std::string, std::string> out;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        out[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }
    return out;
}

std::string getenvOr(const std::unordered_map<std::string, std::string>& file,
                     const char* name, const std::string& fallback = "") {
    if (const char* v = std::getenv(name); v && *v) return v;
    auto it = file.find(name);
    return it != file.end() ? it->second : fallback;
}

} // namespace

Config Config::load() {
    Config cfg;
    // .env may live in the repo root while the exe runs from build*/ —
    // search up to 4 parent dirs.
    std::unordered_map<std::string, std::string> env;
    {
        std::filesystem::path dir = std::filesystem::current_path();
        for (int i = 0; i < 4; ++i) {
            auto p = dir / ".env";
            if (std::filesystem::exists(p)) {
                env = parseEnvFile(p.string());
                break;
            }
            if (!dir.has_parent_path()) break;
            dir = dir.parent_path();
        }
    }

    cfg.api_key = getenvOr(env, "ASSEMBLYAI_API_KEY");
    cfg.hf_token = getenvOr(env, "HF_TOKEN");
    cfg.ws_endpoint = getenvOr(env, "AAI_WS_ENDPOINT", cfg.ws_endpoint);
    cfg.speech_model = getenvOr(env, "AAI_SPEECH_MODEL", cfg.speech_model);
    cfg.diar_model_path = getenvOr(env, "DIAR_MODEL_PATH", cfg.diar_model_path);
    // Relative model paths resolve against the repo root even when the exe
    // runs from a build dir — same upward walk as .env above.
    if (!cfg.diar_model_path.empty() &&
        !std::filesystem::exists(cfg.diar_model_path)) {
        std::filesystem::path dir = std::filesystem::current_path();
        for (int i = 0; i < 4; ++i) {
            if (std::filesystem::exists(dir / cfg.diar_model_path)) {
                cfg.diar_model_path = (dir / cfg.diar_model_path).string();
                break;
            }
            if (!dir.has_parent_path()) break;
            dir = dir.parent_path();
        }
    }
    return cfg;
}

} // namespace ss
