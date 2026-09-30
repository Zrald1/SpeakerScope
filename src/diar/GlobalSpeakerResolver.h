#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>
#include "core/Types.h"

namespace ss {

// Maps Nemotron's local channels (0..7, arrival-ordered, reusable) onto
// unlimited global persons via turn-level speaker embeddings.
//
// resolve(turn, embedding) -> global person index (0..N-1)
// A new person is created when no existing centroid matches above the
// cosine threshold — this is what lifts the 8-speaker ceiling: when
// Nemotron recycles a channel for a 9th voice, the embedding mismatch
// is caught here and spawns a fresh person instead of misattributing.
class GlobalSpeakerResolver {
public:
    struct Options {
        float match_cosine = 0.72f;      // similarity needed to reuse a person
        uint64_t min_turn_ms = 700;      // shorter turns aren't resolved
        size_t centroid_cap = 8;         // utterances averaged per person
        float channel_match_cosine = 0.65f; // looser bar when re-checking a channel
    };

    GlobalSpeakerResolver();
    explicit GlobalSpeakerResolver(Options opts);

    // Returns global person index, or -1 if the turn is too short to resolve
    // (caller should keep showing the local channel label for it).
    int resolve(const SpeakerTurn& turn, const std::vector<float>& embedding);

    int personCount() const;
    // local channel -> last assigned global person (for UI lane labels)
    int globalForChannel(int channel) const;

private:
    static float cosine(const std::vector<float>& a,
                        const std::vector<float>& b);

    Options opts_;
    mutable std::mutex m_;
    // per person: running mean centroid + count of merged utterances
    std::vector<std::vector<float>> centroids_;
    std::vector<size_t> counts_;
    std::array<int, kMaxSpeakers> channel_to_global_{};
};

} // namespace ss
