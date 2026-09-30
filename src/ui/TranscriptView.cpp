#include "ui/TranscriptView.h"

#include <imgui.h>
#include <string>
#include "diar/SpeakerRegistry.h"
#include "ui/Theme.h"

namespace ss {

std::string TranscriptView::joinForCopy(
    const std::vector<AttributedSegment>& segs,
    const SpeakerRegistry& registry, int filter) {
    std::string out;
    for (const auto& s : segs) {
        if (filter >= 0 && s.speaker != filter) continue;
        if (s.speaker >= 0) {
            out += "[" + registry.name(s.speaker) + "] ";
        }
        out += s.text + "\n";
    }
    return out;
}

void TranscriptView::render(const std::vector<AttributedSegment>& segments,
                            SpeakerRegistry& registry) {
    // --- Toolbar: filter combo + copy button --------------------------
    ImGui::TextUnformatted("Show:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160);
    if (ImGui::BeginCombo("##filter",
                        filter_ < 0 ? "Everyone"
                                    : registry.name(filter_).c_str())) {
        if (ImGui::Selectable("Everyone", filter_ < 0)) filter_ = -1;
        for (int ch = 0; ch < kMaxSpeakers; ++ch) {
            if (!registry.seen(ch)) continue;
            ImGui::PushID(1000 + ch);
            ImVec4 c = theme::speakerColor(registry.color(ch));
            ImGui::PushStyleColor(ImGuiCol_Text, c);
            if (ImGui::Selectable(registry.name(ch).c_str(), filter_ == ch))
                filter_ = ch;
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    std::string copy_label =
        filter_ < 0 ? "Copy transcript" : "Copy this person's lines";
    if (ImGui::Button(copy_label.c_str())) {
        ImGui::SetClipboardText(
            joinForCopy(segments, registry, filter_).c_str());
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(edit names in the lanes above)");

    ImGui::Separator();

    // --- Scrolling transcript ------------------------------------------
    ImGui::BeginChild("transcript", ImVec2(0, 0), true,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);

    bool any = false;
    for (const auto& seg : segments) {
        if (filter_ >= 0 && seg.speaker != filter_) continue;
        any = true;

        ImVec4 col = seg.partial ? theme::kPartial
                                 : (seg.speaker >= 0
                                        ? theme::speakerColor(
                                              registry.color(seg.speaker))
                                        : ImVec4(0.9f, 0.9f, 0.92f, 1.f));

        if (seg.speaker >= 0) {
            ImGui::TextColored(col, "[%s]",
                               registry.name(seg.speaker).c_str());
            ImGui::SameLine(0, 6);
        }
        ImGui::PushTextWrapPos(0.f);
        ImGui::TextColored(col, "%s", seg.text.c_str());
        ImGui::PopTextWrapPos();
        if (seg.overlap) {
            ImGui::SameLine(0, 8);
            ImGui::TextColored(theme::kOverlap, "(overlap)");
        }
        ImGui::Spacing();
    }

    if (!any) {
        ImGui::TextDisabled(filter_ < 0
                                ? "Transcript will appear here.\nPick a "
                                  "source on the left and press Start."
                                : "No lines from this person yet.");
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 40.f)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

} // namespace ss
