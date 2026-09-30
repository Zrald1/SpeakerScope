#include "ui/ControlPanel.h"

#include <imgui.h>
#include <cstring>
#include <fstream>

#include "align/AttributedTranscript.h"
#include "app/SessionController.h"
#include "core/Clock.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "ui/Theme.h"

namespace ss {

namespace {

void exportTranscript(SessionController& session) {
    AttributedTranscript doc(session.transcript(), session.timeline());
    std::ofstream f("transcript_export.txt", std::ios::trunc);
    f << doc.renderText(session.registry());
    log()->info("Exported transcript to transcript_export.txt");
}

} // namespace

void ControlPanel::render(SessionController& session, bool api_key_present) {
    const SessionState st = session.state();
    const bool running = st == SessionState::Live || st == SessionState::Connecting;

    ImGui::TextUnformatted("AUDIO SOURCE");
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
        ImGui::TextColored(theme::kOverlap,
                           "ASSEMBLYAI_API_KEY missing\n(set it in .env)");

    ImGui::BeginDisabled(running || !api_key_present);
    if (ImGui::Button("START SESSION", ImVec2(-1, 40)))
        session.start(Config::load(), source_, wav_path_);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!running);
    if (ImGui::Button("STOP", ImVec2(-1, 32))) session.stop();
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::TextDisabled("Status: %s", session.statusLine().c_str());
    ImGui::TextDisabled("Elapsed: %.1f s", session.elapsedMs() / 1000.0);
    ImGui::TextDisabled("Speakers seen: %d", session.registry().speakerCount());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginDisabled(session.transcript().finalizedTurns().empty());
    if (ImGui::Button("Export .txt", ImVec2(-1, 0)))
        exportTranscript(session);
    ImGui::EndDisabled();
}

} // namespace ss
