#include <exception>

#include "app/Application.h"
#include "core/Config.h"
#include "core/Logger.h"

int main(int /*argc*/, char** /*argv*/) {
    ss::initLogger();
    ss::log()->info("SpeakerScope starting");

    try {
        ss::Config cfg = ss::Config::load();
        if (!cfg.valid()) {
            ss::log()->warn("ASSEMBLYAI_API_KEY not set — copy .env.example "
                            "to .env and fill it in");
        }
        ss::Application app(std::move(cfg));
        return app.run();
    } catch (const std::exception& e) {
        ss::log()->critical("fatal: {}", e.what());
        return 1;
    }
}
