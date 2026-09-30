#include "ui/ControlPanel.h"

#include <imgui.h>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>

#include "align/AttributedTranscript.h"
#include "app/SessionController.h"
#include "core/Clock.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "ui/Theme.h"
#include "ui/TranscriptView.h"

namespace ss {

namespace {

std::string sanitizeFilename(const std::string& s) {
    std::string out;
    for (char c : s)
        out += std::isalnum(static_cast<unsigned char>(c)) ? c : '_';
    return out.empty() ? "speaker" : out;
}

// Writes exports/transcript-<ts>.txt (combined, speaker-attributed) plus one
// transcript-<ts>-<name>.txt per seen speaker. Returns the directory used.
std::string exportTranscript(SessionController& session) {
    namespace fs = std::filesystem;
    fs::create_directories("exports");
    AttributedTranscript doc(session.transcript(), session.timeline());
    const auto segs = doc.snapshot();

    char ts[32];
    const std::time_t now = std::time(nullptr);
    std::strftime(ts, sizeof ts, "%Y%m%d-%H%M%S", std::localtime(&now));
    const std::string base = "exports/transcript-" + std::string(ts);

    {
        std::ofstream f(base + ".txt", std::ios::trunc);
        f << doc.renderText(session.registry());
    }
    int per_person = 0;
    for (int ch = 0; ch < kMaxSpeakers; ++ch) {
        if (!session.registry().seen(ch)) continue;
        std::ofstream f(base + "-" +
                            sanitizeFilename(session.registry().name(ch)) +
                            ".txt",
                        std::ios::trunc);
        f << TranscriptView::joinForCopy(segs, session.registry(), ch);
        ++per_person;
    }
    log()->info("Exported {} (+ {} per-person files) to exports/",
                base + ".txt", per_person);
    return base;
}

} // namespace

void ControlPanel::render(SessionController& session, bool api_key_present) {
    const SessionState st = session.state();
    const bool running = st == SessionState::Live || st == SessionState::Connecting;

    ImGui::TextColored(theme::kMuted, "AUDIO SOURCE");
    ImGui::Spacing();

    int sel = static_cast<int>(source_);
    ImGui::BeginDisabled(running);
    if (ImGui::RadioButton("Microphone", &sel, 0))
        source_ = AudioSourceType::Microphone;
    if (ImGui::RadioButton("System audio (loopback)", &sel, 1))
        source_ = AudioSourceType::Loopback;
    if (ImGui::RadioButton("WAV file", &sel, 2))
        source_ = AudioSourceType::WavFile;
    if (source_ == AudioSourceType::WavFile) {
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##wav", wav_buf_, sizeof(wav_buf_)))
            wav_path_ = wav_buf_;
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!api_key_present)
        ImGui::TextColored(theme::kDanger,
                           "ASSEMBLYAI_API_KEY missing\n(set it in .env)");

    // Primary action — solid accent.
    ImGui::PushStyleColor(ImGuiCol_Button, theme::kAccentDim);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme::kAccent);
    ImGui::BeginDisabled(running || !api_key_present);
    if (ImGui::Button("START SESSION", ImVec2(-1, 44)))
        session.start(Config::load(), source_, wav_path_);
    ImGui::EndDisabled();
    ImGui::PopStyleColor(2);

    ImGui::PushStyleColor(ImGuiCol_Button, theme::kDangerDim);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme::kDanger);
    ImGui::BeginDisabled(!running);
    if (ImGui::Button("STOP", ImVec2(-1, 36))) session.stop();
    ImGui::EndDisabled();
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(theme::kMuted, "SESSION");
    ImGui::Spacing();
    ImGui::TextDisabled("Status:   %s", session.statusLine().c_str());
    ImGui::TextDisabled("Elapsed:  %.1f s", session.elapsedMs() / 1000.0);
    ImGui::TextDisabled("Speakers: %d", session.registry().speakerCount());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(theme::kMuted, "EXPORT");
    ImGui::Spacing();
    ImGui::BeginDisabled(session.transcript().finalizedTurns().empty());
    if (ImGui::Button("Export .txt (all + per person)", ImVec2(-1, 0)))
        last_export_ = exportTranscript(session);
    ImGui::EndDisabled();
    if (!last_export_.empty())
        ImGui::TextDisabled("Saved:\n%s*.txt", last_export_.c_str());
}

} // namespace ss
