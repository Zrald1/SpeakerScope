#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include "core/Types.h"

namespace ss {

// Maps anonymous arrival-ordered channels (0..7) to display names and colors.
// Okabe-Ito colorblind-safe palette.
class SpeakerRegistry {
public:
    static constexpr std::array<uint32_t, kMaxSpeakers> kPalette = {
        0xFFE57A00, // orange        (ABGR)
        0xFFDBCCE5, // sky blue
        0xFF599400, // bluish green
        0xFFFFE600, // yellow
        0xFF7856E6, // blue
        0xFF7AB0F0, // vermillion-ish
        0xFFB49E5B, // reddish purple
        0xFF555555, // grey
    };

    const std::string& name(int channel) const;
    void rename(int channel, std::string display_name);
    uint32_t color(int channel) const;
    int speakerCount() const;
    bool seen(int channel) const;
    void noteActive(int channel);

private:
    mutable std::mutex m_;
    std::array<std::string, kMaxSpeakers> names_{
        "Person 1", "Person 2", "Person 3", "Person 4",
        "Person 5", "Person 6", "Person 7", "Person 8"};
    std::array<bool, kMaxSpeakers> seen_{};
};

} // namespace ss
