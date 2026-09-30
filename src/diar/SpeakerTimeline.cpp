#include "diar/SpeakerTimeline.h"

#include <algorithm>

namespace ss {

void SpeakerTimeline::addFrame(const ActivityFrame& f) {
    std::lock_guard lk(m_);
    frames_.push_back(f);
    while (frames_.size() > 1 &&
           f.start_ms > opts_.window_ms &&
           frames_.front().start_ms < f.start_ms - opts_.window_ms) {
        frames_.pop_front();
    }
    // Materialize only up to a guard distance behind the live edge so a
    // currently-open turn isn't closed prematurely.
    uint64_t up_to =
        f.start_ms > opts_.close_guard_ms ? f.start_ms - opts_.close_guard_ms
                                          : 0;
    materializeTurns(up_to);
}

void SpeakerTimeline::materializeTurns(uint64_t up_to_ms) {
    auto emit = [&](int ch, uint64_t end) {
        auto& o = open_[ch];
        if (!o.active) return;
        o.active = false;
        if (end <= o.start || end - o.start < opts_.min_turn_ms) {
            o.n = 0; o.acc = 0;
            return;
        }
        SpeakerTurn t{ch, o.start, end, o.acc / static_cast<float>(o.n)};
        if (!turns_.empty() && turns_.back().speaker == ch &&
            t.start_ms <= turns_.back().end_ms + opts_.merge_gap_ms) {
            auto& last = turns_.back();
            last.end_ms = t.end_ms;
            last.confidence = std::max(last.confidence, t.confidence);
        } else {
            turns_.push_back(t);
        }
        o.n = 0; o.acc = 0;
    };

    for (const auto& f : frames_) {
        if (f.start_ms < materialized_up_to_) continue;
        if (f.start_ms >= up_to_ms) break;
        for (int ch = 0; ch < kMaxSpeakers; ++ch) {
            auto& o = open_[ch];
            if (f.probs[ch] >= opts_.active_threshold) {
                if (!o.active) { o.active = true; o.start = f.start_ms; o.acc = 0; o.n = 0; }
                o.acc += f.probs[ch];
                ++o.n;
            } else if (o.active) {
                emit(ch, f.start_ms);
            }
        }
    }
    materialized_up_to_ = std::max(materialized_up_to_, up_to_ms);
}

void SpeakerTimeline::addTurn(const SpeakerTurn& t) {
    std::lock_guard lk(m_);
    if (t.speaker < 0 || t.end_ms <= t.start_ms ||
        t.end_ms - t.start_ms < opts_.min_turn_ms) {
        return;
    }
    auto it = std::lower_bound(
        turns_.begin(), turns_.end(), t.start_ms,
        [](const SpeakerTurn& a, uint64_t s) { return a.start_ms < s; });
    // Merge with an adjacent same-speaker turn within merge_gap.
    if (it != turns_.begin()) {
        auto& prev = *std::prev(it);
        if (prev.speaker == t.speaker &&
            t.start_ms <= prev.end_ms + opts_.merge_gap_ms) {
            prev.end_ms = std::max(prev.end_ms, t.end_ms);
            prev.confidence = std::max(prev.confidence, t.confidence);
            return;
        }
    }
    if (it != turns_.end() && it->speaker == t.speaker &&
        it->start_ms <= t.end_ms + opts_.merge_gap_ms) {
        it->start_ms = std::min(it->start_ms, t.start_ms);
        it->end_ms = std::max(it->end_ms, t.end_ms);
        it->confidence = std::max(it->confidence, t.confidence);
        return;
    }
    turns_.insert(it, t);
}

const ActivityFrame* SpeakerTimeline::frameAt(uint64_t ms) const {
    // Find the latest frame starting at or before ms (frames are appended in
    // order, stride 10 ms).
    const ActivityFrame* hit = nullptr;
    for (const auto& f : frames_) {
        if (f.start_ms > ms) break;
        hit = &f;
    }
    if (hit && ms - hit->start_ms <= 100) return hit; // stale data guard
    return nullptr;
}

int SpeakerTimeline::dominantSpeakerAt(uint64_t ms) const {
    std::lock_guard lk(m_);
    if (const ActivityFrame* f = frameAt(ms)) {
        int best = -1;
        float best_p = opts_.active_threshold;
        for (int ch = 0; ch < kMaxSpeakers; ++ch)
            if (f->probs[ch] > best_p) { best_p = f->probs[ch]; best = ch; }
        return best;
    }
    // Turn-fed path: highest-confidence turn covering ms.
    int best = -1;
    float best_c = 0.f;
    for (const auto& t : turns_) {
        if (t.start_ms > ms) break;
        if (ms < t.end_ms && t.confidence > best_c) {
            best_c = t.confidence;
            best = t.speaker;
        }
    }
    return best;
}

int SpeakerTimeline::activeSpeakerCountAt(uint64_t ms) const {
    std::lock_guard lk(m_);
    if (const ActivityFrame* f = frameAt(ms)) {
        int n = 0;
        for (int ch = 0; ch < kMaxSpeakers; ++ch)
            if (f->probs[ch] >= opts_.active_threshold) ++n;
        return n;
    }
    int n = 0;
    for (const auto& t : turns_) {
        if (t.start_ms > ms) break;
        if (ms < t.end_ms) ++n;
    }
    return n;
}

std::vector<SpeakerTurn> SpeakerTimeline::turns() const {
    std::lock_guard lk(m_);
    return turns_;
}

std::vector<ActivityFrame> SpeakerTimeline::framesSince(uint64_t ms) const {
    std::lock_guard lk(m_);
    std::vector<ActivityFrame> out;
    for (const auto& f : frames_)
        if (f.start_ms >= ms) out.push_back(f);
    return out;
}

void SpeakerTimeline::clear() {
    std::lock_guard lk(m_);
    frames_.clear();
    turns_.clear();
    open_.fill(ChannelOpen{});
    materialized_up_to_ = 0;
}

} // namespace ss
