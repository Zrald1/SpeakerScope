#pragma once

#include <string>
#include "core/Types.h"

namespace ss {

class SessionController;

// Left column: source picker, start/stop, session stats, export.
class ControlPanel {
public:
    void render(SessionController& session, bool api_key_present);

    AudioSourceType selectedSource() const { return source_; }
    const std::string& wavPath() const { return wav_path_; }

private:
    AudioSourceType source_ = AudioSourceType::Microphone;
    std::string wav_path_ = "assets/samples/meeting.wav";
    char wav_buf_[512] = "assets/samples/meeting.wav";
    std::string last_export_;
};

} // namespace ss
