#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace ss {

// Call once in main() before any subsystem starts.
void initLogger();

// Falls back to the default logger when initLogger() hasn't run (tests).
inline std::shared_ptr<spdlog::logger> log() {
    if (auto l = spdlog::get("speakerscope")) return l;
    return spdlog::default_logger();
}

} // namespace ss
