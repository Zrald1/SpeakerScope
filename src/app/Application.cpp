#include "app/Application.h"

#include <cmath>
#include <cstdio>
#include <imgui.h>

#include "align/AttributedTranscript.h"
#include "core/Logger.h"
#include "ui/AppWindow.h"
#include "ui/ControlPanel.h"
#include "ui/SpeakerLanes.h"
#include "ui/Theme.h"
#include "ui/TranscriptView.h"

namespace ss {

namespace {

std::string clockString(uint64_t ms) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02llu:%02llu",
                  (unsigned long long)(ms / 60000),
                  (unsigned long long)((ms / 1000) % 60));
    return buf;
}

void sectionHeader(const char* text) {
    ImGui::TextColored(theme::kMuted, "%s", text);
    ImGui::Spacing();
}

} // namespace

Application::Application(Config cfg) : cfg_(std::move(cfg)) {}
Application::~Application() = default;

int Application::run() {
    window_ = std::make_unique<AppWindow>(1280, 800, "SpeakerScope");

    ControlPanel controls;
    SpeakerLanes lanes;
    TranscriptView transcript_view;
    AttributedTranscript doc(session_.transcript(), session_.timeline());

    while (!window_->shouldClose()) {
        window_->beginFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("SpeakerScope", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);

        // ---- Header bar -------------------------------------------------
        const SessionState st = session_.state();
        const bool live = st == SessionState::Live;

        if (window_->titleFont()) ImGui::PushFont(window_->titleFont());
        ImGui::TextColored(theme::kAccent, "SpeakerScope");
        if (window_->titleFont()) ImGui::PopFont();
        ImGui::SameLine();
        ImGui::TextDisabled("live speaker-attributed transcription");

        // Right-aligned status: pulsing LIVE dot + clock + speaker count.
        const float right_w = 330.f;
        ImGui::SameLine(ImGui::GetWindowWidth() - right_w);
        ImVec2 p = ImGui::GetCursorScreenPos();
        auto* dl = ImGui::GetWindowDrawList();
        const float cy = p.y + ImGui::GetTextLineHeight() * 0.5f;
        const ImVec4 dot_col = live ? theme::kLive : theme::kFaint;
        if (live) {
            // soft pulse ring
            const float pulse =
                0.5f + 0.5f * std::sin(float(ImGui::GetTime()) * 3.f);
            dl->AddCircleFilled(ImVec2(p.x + 5, cy), 7.f + 3.f * pulse,
                                ImGui::ColorConvertFloat4ToU32(
                                    ImVec4(dot_col.x, dot_col.y, dot_col.z,
                                           0.18f * pulse)));
        }
        dl->AddCircleFilled(ImVec2(p.x + 5, cy), 4.5f,
                            ImGui::ColorConvertFloat4ToU32(dot_col));
        ImGui::Dummy(ImVec2(14, 0));
        ImGui::SameLine();
        ImGui::TextColored(live ? theme::kLive : theme::kMuted,
                           live ? "LIVE" : "IDLE");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextUnformatted(clockString(session_.elapsedMs()).c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(theme::kMuted, "%d speakers",
                           session_.registry().speakerCount());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- Body -------------------------------------------------------
        ImGui::BeginChild("controls", ImVec2(292, 0), true);
        controls.render(session_, cfg_.valid());
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("main", ImVec2(0, 0), false,
                          ImGuiWindowFlags_NoBackground);
        sectionHeader("SPEAKER ACTIVITY");
        ImGui::BeginChild("lanes", ImVec2(0, 222), true);
        // Frame-fed engines don't fire the turn callback — mark channels seen
        // from materialized turns so the person filter + count stay live.
        for (const auto& t : session_.timeline().turns())
            session_.registry().noteActive(t.speaker);
        lanes.render(session_.timeline(), session_.registry(),
                     session_.elapsedMs());
        ImGui::EndChild();

        ImGui::Spacing();
        sectionHeader("TRANSCRIPT");
        ImGui::BeginChild("transcript_card", ImVec2(0, 0), true);
        transcript_view.render(doc.snapshot(), session_.registry());
        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::End();
        window_->endFrame();
    }

    session_.stop();
    return 0;
}

} // namespace ss
