#pragma once

#include <imgui.h>
#include "core/Types.h"

namespace ss::theme {

inline ImVec4 abgr(uint32_t c) {
    return ImVec4((c & 0xFF) / 255.f, ((c >> 8) & 0xFF) / 255.f,
                  ((c >> 16) & 0xFF) / 255.f, ((c >> 24) & 0xFF) / 255.f);
}

inline ImVec4 speakerColor(uint32_t abgrColor) { return abgr(abgrColor); }

constexpr ImVec4 kPartial{0.62f, 0.62f, 0.66f, 1.0f}; // dim grey for partials
constexpr ImVec4 kOverlap{1.0f, 0.45f, 0.2f, 1.0f};
constexpr ImVec4 kBg{0.075f, 0.08f, 0.095f, 1.0f};
constexpr ImVec4 kPanel{0.12f, 0.13f, 0.15f, 1.0f};
constexpr ImVec4 kAccent{0.30f, 0.65f, 1.0f, 1.0f};

} // namespace ss::theme
