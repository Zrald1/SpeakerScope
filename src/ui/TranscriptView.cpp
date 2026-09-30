#include "ui/TranscriptView.h"

#include <imgui.h>
#include <string>
#include "diar/SpeakerRegistry.h"
#include "ui/Theme.h"

namespace ss {

namespace {

std::string tsString(uint64_t ms) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02llu:%02llu",
                  (unsigned long long)(ms / 60000),
                  (unsigned long long)((ms / 1000) % 60));
    return buf;
}

} // namespace

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
    ImGui::TextColored(theme::kMuted, "Show:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(170);
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
    ImGui::TextDisabled("rename people in the lanes above");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- Scrolling transcript ------------------------------------------
    ImGui::BeginChild("transcript", ImVec2(0, 0), false,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    bool any = false;
    for (const auto& seg : segments) {
        if (filter_ >= 0 && seg.speaker != filter_) continue;
        any = true;

        // Timestamp gutter.
        ImGui::TextColored(theme::kFaint, "%s", tsString(seg.start_ms).c_str());
        ImGui::SameLine(52);

        if (seg.speaker >= 0) {
            // Speaker chip: tinted pill + colored name.
            ImVec4 col = theme::speakerColor(registry.color(seg.speaker));
            const std::string nm = registry.name(seg.speaker);
            const ImVec2 tsz = ImGui::CalcTextSize(nm.c_str());
            ImVec2 chip_p = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(
                chip_p, ImVec2(chip_p.x + tsz.x + 14,
                               chip_p.y + tsz.y + 6),
                ImGui::ColorConvertFloat4ToU32(
                    ImVec4(col.x, col.y, col.z, seg.partial ? 0.10f : 0.22f)),
                tsz.y * 0.5f + 3.f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 7);
            ImGui::TextColored(ImVec4(col.x, col.y, col.z,
                                      seg.partial ? 0.7f : 1.f),
                               "%s", nm.c_str());
            ImGui::SameLine(0, 13);
        } else {
            ImGui::TextColored(theme::kMuted, "[?]");
            ImGui::SameLine(0, 6);
        }

        ImVec4 col = seg.partial ? theme::kPartial
                                 : (seg.speaker >= 0
                                        ? theme::kText
                                        : ImVec4(0.85f, 0.85f, 0.87f, 1.f));
        ImGui::PushTextWrapPos(0.f);
        ImGui::TextColored(col, "%s", seg.text.c_str());
        ImGui::PopTextWrapPos();
        if (seg.overlap) {
            ImGui::SameLine(0, 8);
            ImGui::TextColored(theme::kOverlap, "overlap");
        }
        ImGui::Spacing();
    }

    if (!any) {
        ImGui::Spacing();
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
