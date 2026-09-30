#include "app/Application.h"

#include <imgui.h>

#include "align/AttributedTranscript.h"
#include "core/Logger.h"
#include "ui/AppWindow.h"
#include "ui/ControlPanel.h"
#include "ui/SpeakerLanes.h"
#include "ui/Theme.h"
#include "ui/TranscriptView.h"

namespace ss {

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
                         ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(theme::kAccent, "SpeakerScope");
        ImGui::SameLine();
        ImGui::TextDisabled("— live speaker-attributed transcription");
        ImGui::Separator();

        ImGui::BeginChild("left", ImVec2(280, 0), true);
        controls.render(session_, cfg_.valid());
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("right", ImVec2(0, 0), false);
        ImGui::TextUnformatted("SPEAKER ACTIVITY");
        ImGui::BeginChild("lanes", ImVec2(0, 150), true);
        lanes.render(session_.timeline(), session_.registry(),
                     session_.elapsedMs());
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::TextUnformatted("TRANSCRIPT");
        transcript_view.render(doc.snapshot(), session_.registry());
        ImGui::EndChild();

        ImGui::End();
        window_->endFrame();
    }

    session_.stop();
    return 0;
}

} // namespace ss
