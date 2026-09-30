#pragma once

#include <imgui.h>
#include "core/Types.h"

namespace ss::theme {

inline ImVec4 abgr(uint32_t c) {
    return ImVec4((c & 0xFF) / 255.f, ((c >> 8) & 0xFF) / 255.f,
                  ((c >> 16) & 0xFF) / 255.f, ((c >> 24) & 0xFF) / 255.f);
}

inline ImVec4 speakerColor(uint32_t abgrColor) { return abgr(abgrColor); }

// Refined dark palette — calm surfaces, one teal accent, muted secondaries.
constexpr ImVec4 kBg{0.051f, 0.059f, 0.075f, 1.0f};        // #0D0F13 deep canvas
constexpr ImVec4 kPanel{0.082f, 0.094f, 0.114f, 1.0f};     // #15181D card
constexpr ImVec4 kPanelAlt{0.102f, 0.114f, 0.137f, 1.0f};  // elevated surface
constexpr ImVec4 kTrack{0.067f, 0.075f, 0.090f, 1.0f};     // sunken lane track
constexpr ImVec4 kBorder{0.180f, 0.200f, 0.235f, 1.0f};
constexpr ImVec4 kText{0.910f, 0.930f, 0.953f, 1.0f};
constexpr ImVec4 kMuted{0.545f, 0.580f, 0.620f, 1.0f};
constexpr ImVec4 kFaint{0.360f, 0.390f, 0.430f, 1.0f};

constexpr ImVec4 kAccent{0.204f, 0.761f, 0.859f, 1.0f};    // teal #34C2DB
constexpr ImVec4 kAccentDim{0.110f, 0.400f, 0.470f, 1.0f};
constexpr ImVec4 kAccentSoft{0.204f, 0.761f, 0.859f, 0.16f};

constexpr ImVec4 kLive{0.290f, 0.855f, 0.470f, 1.0f};      // live green
constexpr ImVec4 kDanger{0.918f, 0.373f, 0.325f, 1.0f};    // warm coral red
constexpr ImVec4 kDangerDim{0.480f, 0.200f, 0.180f, 1.0f};

constexpr ImVec4 kPartial{0.560f, 0.580f, 0.620f, 0.85f};  // dimmed partials
constexpr ImVec4 kOverlap{1.000f, 0.620f, 0.270f, 1.0f};   // amber overlap flag

} // namespace ss::theme
