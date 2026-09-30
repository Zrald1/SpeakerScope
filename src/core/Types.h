#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ss {

// Single canonical audio format for the whole app.
constexpr int kSampleRate = 16000;
constexpr int kChannels = 1;
constexpr int kBytesPerSample = 2; // s16le
constexpr int kChunkMs = 50;       // AssemblyAI recommended frame size
constexpr int kChunkSamples = kSampleRate * kChunkMs / 1000; // 800 samples / 1600 bytes

constexpr int kMaxSpeakers = 8;    // Nemotron-3-Diarization channel count

enum class AudioSourceType { Microphone, Loopback, WavFile };

// A word inside an AssemblyAI Turn message (timestamps in ms of stream time).
struct Word {
    std::string text;
    uint32_t start_ms = 0;
    uint32_t end_ms = 0;
    float confidence = 0.0f;
    bool word_is_final = false;
};

// An AssemblyAI Turn event. Partial turns (end_of_turn=false) are revised in place.
struct Turn {
    uint32_t turn_order = 0;
    std::string transcript;
    bool end_of_turn = false;
    double end_of_turn_confidence = 0.0;
    std::vector<Word> words;
};

// One speaker turn from the diarization engine ("speaker N was active a..b").
struct SpeakerTurn {
    int speaker = -1; // arrival-ordered channel 0..7
    uint64_t start_ms = 0;
    uint64_t end_ms = 0;
    float confidence = 0.0f;
};

// Final fused output unit rendered by the UI.
struct AttributedSegment {
    int speaker = -1; // -1 = unknown/no diarization data
    std::string text;
    uint64_t start_ms = 0;
    uint64_t end_ms = 0;
    bool overlap = false;   // 2+ speakers active during this segment
    bool partial = false;   // belongs to a non-finalized turn
};

enum class SessionState { Idle, Connecting, Live, Stopping, Error };

} // namespace ss
