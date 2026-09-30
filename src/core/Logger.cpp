#include "core/Logger.h"

#include <filesystem>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace ss {

void initLogger() {
    std::filesystem::create_directories("logs");

    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console->set_level(spdlog::level::info);

    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/speakerscope.log", 5 * 1024 * 1024, 3);
    file->set_level(spdlog::level::debug);

    auto logger = std::make_shared<spdlog::logger>(
        "speakerscope", spdlog::sinks_init_list{console, file});
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%H:%M:%S.%e] [%l] [%t] %v");
    // set_default_logger already registers it under "speakerscope".
    spdlog::set_default_logger(std::move(logger));
}

} // namespace ss
