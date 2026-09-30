#include "diar/GlobalSpeakerResolver.h"

#include <algorithm>
#include <cmath>

namespace ss {

GlobalSpeakerResolver::GlobalSpeakerResolver() {
    channel_to_global_.fill(-1);
}

GlobalSpeakerResolver::GlobalSpeakerResolver(Options opts) : opts_(opts) {
    channel_to_global_.fill(-1);
}

float GlobalSpeakerResolver::cosine(const std::vector<float>& a,
                                    const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.f;
    float dot = 0.f, na = 0.f, nb = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    if (na <= 0.f || nb <= 0.f) return 0.f;
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

int GlobalSpeakerResolver::resolve(const SpeakerTurn& turn,
                                   const std::vector<float>& emb) {
    if (emb.empty()) return -1;
    std::lock_guard lk(m_);

    if (turn.end_ms - turn.start_ms < opts_.min_turn_ms)
        return turn.speaker >= 0 && turn.speaker < kMaxSpeakers
                   ? channel_to_global_[turn.speaker]
                   : -1;

    int best = -1;
    float best_sim = opts_.match_cosine;
    for (size_t i = 0; i < centroids_.size(); ++i) {
        float sim = cosine(emb, centroids_[i]);
        if (sim > best_sim) {
            best_sim = sim;
            best = static_cast<int>(i);
        }
    }

    int person;
    if (best >= 0) {
        person = best;
        // running-mean centroid update (capped influence)
        auto& c = centroids_[person];
        size_t n = counts_[person];
        float w = 1.f / static_cast<float>(std::min(n + 1, opts_.centroid_cap));
        for (size_t d = 0; d < c.size(); ++d)
            c[d] = c[d] * (1.f - w) + emb[d] * w;
        ++counts_[person];
    } else {
        person = static_cast<int>(centroids_.size());
        centroids_.push_back(emb);
        counts_.push_back(1);
    }

    if (turn.speaker >= 0 && turn.speaker < kMaxSpeakers)
        channel_to_global_[turn.speaker] = person;
    return person;
}

int GlobalSpeakerResolver::personCount() const {
    std::lock_guard lk(m_);
    return static_cast<int>(centroids_.size());
}

int GlobalSpeakerResolver::globalForChannel(int channel) const {
    std::lock_guard lk(m_);
    if (channel < 0 || channel >= kMaxSpeakers) return -1;
    return channel_to_global_[channel];
}

} // namespace ss
