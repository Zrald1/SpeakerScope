#include "diar/SpeakerRegistry.h"

namespace ss {

const std::string& SpeakerRegistry::name(int channel) const {
    std::lock_guard lk(m_);
    static const std::string unknown = "Unknown";
    if (channel < 0 || channel >= kMaxSpeakers) return unknown;
    return names_[channel];
}

void SpeakerRegistry::rename(int channel, std::string display_name) {
    std::lock_guard lk(m_);
    if (channel >= 0 && channel < kMaxSpeakers && !display_name.empty())
        names_[channel] = std::move(display_name);
}

uint32_t SpeakerRegistry::color(int channel) const {
    if (channel < 0 || channel >= kMaxSpeakers) return 0xFF888888;
    return kPalette[channel];
}

int SpeakerRegistry::speakerCount() const {
    std::lock_guard lk(m_);
    int n = 0;
    for (bool s : seen_) n += s ? 1 : 0;
    return n;
}

bool SpeakerRegistry::seen(int channel) const {
    std::lock_guard lk(m_);
    return channel >= 0 && channel < kMaxSpeakers && seen_[channel];
}

void SpeakerRegistry::noteActive(int channel) {
    std::lock_guard lk(m_);
    if (channel >= 0 && channel < kMaxSpeakers) seen_[channel] = true;
}

} // namespace ss
