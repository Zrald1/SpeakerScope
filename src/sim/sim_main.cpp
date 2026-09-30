// Headless pipeline simulator.
//   speakerscope_sim <wav> [gold.json]
// Streams the wav through AssemblyAI v3 for real. If gold.json is given,
// replays its speaker turns through WavFileDiarizer; otherwise the real
// NemotronDiarizer (audio.cpp + models/nemotron-3-diarization-q8_0.gguf)
// runs on-device against the same audio.

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include <nlohmann/json.hpp>

#include "align/AttributedTranscript.h"
#include "app/SessionController.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "diar/SpeakerRegistry.h"
#include "diar/WavFileDiarizer.h"

using namespace ss;

static std::unique_ptr<DiarizationEngine> loadGold(const std::string& path) {
    std::ifstream f(path);
    nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
    if (j.is_discarded()) return nullptr;
    std::vector<SpeakerTurn> gold;
    for (auto& t : j) {
        gold.push_back({t.value("speaker", -1),
                        t.value("start_ms", uint64_t{0}),
                        t.value("end_ms", uint64_t{0}), 0.95f});
    }
    return std::make_unique<WavFileDiarizer>(std::move(gold));
}

int main(int argc, char** argv) {
    std::string wav = argc > 1 ? argv[1] : "assets/samples/meeting.wav";
    std::string gold = argc > 2 ? argv[2] : "";

    initLogger();
    Config cfg = Config::load();
    if (!cfg.valid()) {
        std::cerr << "ASSEMBLYAI_API_KEY missing (put it in .env)\n";
        return 2;
    }

    SessionController session;
    auto diar = gold.empty() ? nullptr : loadGold(gold);

    std::cout << "== SpeakerScope sim ==\n"
              << "wav:  " << wav << "\n"
              << "gold: " << (gold.empty() ? "(none — plain transcript)" : gold)
              << "\nstreaming to AssemblyAI...\n\n";

    if (!session.start(cfg, AudioSourceType::WavFile, wav, std::move(diar))) {
        std::cerr << "session failed: " << session.statusLine() << "\n";
        return 1;
    }

    // Live view: reprint current partial transcript every 300 ms.
    auto t0 = std::chrono::steady_clock::now();
    std::string last_partial;
    while (session.state() == SessionState::Live ||
           session.state() == SessionState::Connecting) {
        auto partials = session.transcript().partialTurns();
        if (!partials.empty() &&
            partials.back().transcript != last_partial) {
            last_partial = partials.back().transcript;
            std::cout << "\r" << std::string(90, ' ') << "\r"
                      << "live> " << last_partial << std::flush;
        }
        // run until wav done + grace period for final turns
        auto elapsed = std::chrono::steady_clock::now() - t0;
        if (!session.sourceRunning() &&
            elapsed > std::chrono::seconds(4)) {
            // source finished; give AssemblyAI a moment to emit final turns
            std::this_thread::sleep_for(std::chrono::seconds(3));
            break;
        }
        if (elapsed > std::chrono::seconds(180)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    session.stop();
    std::cout << "\n\n===== FINAL ATTRIBUTED TRANSCRIPT =====\n";
    AttributedTranscript doc(session.transcript(), session.timeline());
    for (const auto& seg : doc.snapshot()) {
        if (seg.speaker >= 0)
            std::cout << "[" << session.registry().name(seg.speaker) << "] ";
        else
            std::cout << "[?] ";
        std::cout << seg.text << (seg.overlap ? "  (overlap)" : "") << "\n";
    }
    std::cout << "\n===== PLAIN =====\n" << session.transcript().finalizedText()
              << "\n";
    return 0;
}
