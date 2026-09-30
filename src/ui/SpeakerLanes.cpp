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
    const float lane_h = 20.f;
    const float label_w = 110.f;
    const float gap = 4.f;

    const uint64_t t0 = now_ms > window_ms ? now_ms - window_ms : 0;
    auto turns = timeline.turns();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float lane_w = ImGui::GetContentRegionAvail().x - label_w;

    for (int ch = 0; ch < kMaxSpeakers; ++ch) {
        ImGui::PushID(ch);

        // Lazy-init the edit buffer from the registry name.
        if (!buf_init_[ch]) {
            std::strncpy(name_buf_[ch].data(), registry.name(ch).c_str(), 63);
            name_buf_[ch][63] = '\0';
            buf_init_[ch] = true;
        }

        // Editable name, tinted with the person's color.
        ImVec4 col4 = theme::speakerColor(registry.color(ch));
        ImGui::PushStyleColor(ImGuiCol_Text, col4);
        ImGui::SetNextItemWidth(label_w - 8);
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

        // Lane ribbon.
        ImVec2 origin = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(origin, ImVec2(origin.x + lane_w, origin.y + lane_h),
                          IM_COL32(30, 32, 38, 255), 4.f);

        ImU32 bar = ImGui::ColorConvertFloat4ToU32(col4);
        for (const auto& t : turns) {
            if (t.speaker != ch || t.end_ms < t0) continue;
            float x0 = origin.x +
                       lane_w * float(t.start_ms - t0) / float(window_ms);
            float x1 = origin.x +
                       lane_w *
                           float(std::min<uint64_t>(t.end_ms, now_ms) - t0) /
                           float(window_ms);
            x0 = std::max(x0, origin.x);
            if (x1 > x0)
                dl->AddRectFilled(ImVec2(x0, origin.y + 2),
                                  ImVec2(x1, origin.y + lane_h - 2), bar, 3.f);
        }

        // "now" cursor.
        dl->AddLine(ImVec2(origin.x + lane_w - 1, origin.y),
                    ImVec2(origin.x + lane_w - 1, origin.y + lane_h),
                    IM_COL32(255, 255, 255, 140), 1.f);

        ImGui::Dummy(ImVec2(lane_w, lane_h));
        ImGui::PopID();
    }
}

} // namespace ss
