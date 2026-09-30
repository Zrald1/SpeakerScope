#include "ui/SpeakerLanes.h"

#include <imgui.h>
#include <algorithm>
#include <cstring>
#include "core/Types.h"
#include "diar/SpeakerRegistry.h"
#include "diar/SpeakerTimeline.h"
#include "ui/Theme.h"

namespace ss {

void SpeakerLanes::render(const SpeakerTimeline& timeline,
                          SpeakerRegistry& registry, uint64_t now_ms,
                          uint64_t window_ms) {
    const float lane_h = 22.f;
    const float label_w = 118.f;

    const uint64_t t0 = now_ms > window_ms ? now_ms - window_ms : 0;
    auto turns = timeline.turns();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float lane_w = ImGui::GetContentRegionAvail().x - label_w;

    for (int ch = 0; ch < kMaxSpeakers; ++ch) {
        ImGui::PushID(ch);
        const bool seen = registry.seen(ch);

        // Lazy-init the edit buffer from the registry name.
        if (!buf_init_[ch]) {
            std::strncpy(name_buf_[ch].data(), registry.name(ch).c_str(), 63);
            name_buf_[ch][63] = '\0';
            buf_init_[ch] = true;
        }

        // Channel dot — lit when this person has spoken, faint otherwise.
        ImVec2 dot_pos = ImGui::GetCursorScreenPos();
        ImVec4 col4 = theme::speakerColor(registry.color(ch));
        dl->AddCircleFilled(
            ImVec2(dot_pos.x + 5, dot_pos.y + lane_h * 0.5f + 4.f), 4.f,
            ImGui::ColorConvertFloat4ToU32(
                seen ? col4 : theme::kFaint));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 14);

        // Editable name, tinted with the person's color.
        ImGui::PushStyleColor(ImGuiCol_Text,
                              seen ? col4 : theme::kMuted);
        ImGui::SetNextItemWidth(label_w - 20);
        if (ImGui::InputText("##name", name_buf_[ch].data(),
                             name_buf_[ch].size(),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            registry.rename(ch, name_buf_[ch].data());
        }
        // Commit also when the field loses focus with changed text.
        if (ImGui::IsItemDeactivatedAfterEdit())
            registry.rename(ch, name_buf_[ch].data());
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Lane ribbon — sunken track.
        ImVec2 origin = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(origin,
                          ImVec2(origin.x + lane_w, origin.y + lane_h),
                          ImGui::ColorConvertFloat4ToU32(theme::kTrack), 6.f);
        // hairline border
        dl->AddRect(origin, ImVec2(origin.x + lane_w, origin.y + lane_h),
                    ImGui::ColorConvertFloat4ToU32(
                        ImVec4(theme::kBorder.x, theme::kBorder.y,
                               theme::kBorder.z, 0.4f)),
                    6.f, 0, 1.f);

        const ImU32 bar = ImGui::ColorConvertFloat4ToU32(col4);
        const ImU32 glow = ImGui::ColorConvertFloat4ToU32(
            ImVec4(col4.x, col4.y, col4.z, 0.30f));
        for (const auto& t : turns) {
            if (t.speaker != ch || t.end_ms <= t0) continue;
            // Clamp to the window BEFORE subtracting — uint64 underflow on
            // t.start_ms - t0 threw x0 off-screen, making a still-active
            // turn vanish once its start scrolled past the left edge.
            const uint64_t s = std::max(t.start_ms, t0);
            const uint64_t e = std::min(t.end_ms, now_ms);
            if (e <= s) continue;
            float x0 = origin.x + lane_w * float(s - t0) / float(window_ms);
            float x1 = origin.x + lane_w * float(e - t0) / float(window_ms);
            const bool live = t.end_ms >= now_ms; // still growing
            if (live) // soft glow under live bars
                dl->AddRectFilled(ImVec2(x0, origin.y + 1),
                                  ImVec2(x1, origin.y + lane_h - 1), glow,
                                  6.f);
            dl->AddRectFilled(ImVec2(x0, origin.y + 3),
                              ImVec2(x1, origin.y + lane_h - 3), bar, 5.f);
        }

        // "now" cursor.
        dl->AddLine(ImVec2(origin.x + lane_w - 1, origin.y + 3),
                    ImVec2(origin.x + lane_w - 1, origin.y + lane_h - 3),
                    IM_COL32(255, 255, 255, 110), 1.f);

        ImGui::Dummy(ImVec2(lane_w, lane_h));
        ImGui::PopID();
    }
}

} // namespace ss
