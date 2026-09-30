// Parameterized battery: 80 generated meetings pushed through the turn path
// (addTurn -> dominantSpeakerAt / activeSpeakerCountAt / SpeakerAttributor).
// Turns are shuffled before insertion to simulate closed turns arriving
// late and out of order, exactly as the real streaming decoder emits them.
#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "align/SpeakerAttributor.h"
#include "diar/SpeakerTimeline.h"

using namespace ss;

namespace {

struct Interval { int spk; uint64_t s, e; float conf; };

// Deterministic PRNG per case.
struct Rng {
    explicit Rng(uint32_t seed) : st(seed * 2654435761u + 0x9E3779B9u) {}
    uint32_t next() {
        st ^= st << 13; st ^= st >> 17; st ^= st << 5;
        return st;
    }
    uint32_t range(uint32_t lo, uint32_t hi) { return lo + next() % (hi - lo + 1); }
    uint32_t st;
};

// Generates a plausible meeting: 1-8 speakers, 30-90 s, intervals >=400 ms,
// silence gaps >=400 ms between consecutive utterances, occasional overlap.
std::vector<Interval> genMeeting(Rng& rng) {
    const int speakers = 1 + (rng.next() % 8);
    const uint64_t span_ms = 30'000 + (rng.next() % 60'000);
    std::vector<Interval> out;
    uint64_t t = rng.range(200, 2000);
    int last_spk = -1;
    while (t < span_ms) {
        int spk;
        do { spk = rng.next() % speakers; } while (spk == last_spk && speakers > 1);
        last_spk = spk;
        const uint64_t dur = rng.range(400, 3000);
        out.push_back({spk, t, t + dur, 0.55f + (rng.next() % 40) / 100.f});
        uint64_t last_end = t + dur;
        // 30% chance: a different speaker overlaps the tail of this turn.
        if (speakers > 1 && rng.next() % 10 < 3 && dur > 800) {
            int other;
            do { other = rng.next() % speakers; } while (other == spk);
            const uint64_t os = t + dur - rng.range(300, 600);
            const uint64_t od = rng.range(300, 900);
            last_end = std::max(last_end, os + od);
            out.push_back({other, os, os + od,
                           0.55f + (rng.next() % 40) / 100.f});
        }
        // gap > merge_gap (300 ms) measured from the LATEST turn end, so an
        // overlap turn can never merge with the next same-speaker interval.
        t = last_end + rng.range(400, 1200);
    }
    return out;
}

} // namespace

class TurnBattery : public ::testing::TestWithParam<int> {};

TEST_P(TurnBattery, GeneratedMeetingAttribution) {
    Rng rng(static_cast<uint32_t>(GetParam()));
    const auto intervals = genMeeting(rng);

    // Shuffle delivery order — closed turns arrive whenever they finish.
    std::vector<Interval> shuffled = intervals;
    std::mt19937 mt(GetParam());
    std::shuffle(shuffled.begin(), shuffled.end(), mt);

    SpeakerTimeline tl;
    for (const auto& iv : shuffled)
        tl.addTurn({iv.spk, iv.s, iv.e, iv.conf});

    // Turn count: no two generated intervals of the same speaker are within
    // merge_gap of each other (gaps >=400 ms), so nothing merges or drops.
    EXPECT_EQ(tl.turns().size(), intervals.size());

    // Verify the materialized list is sorted by start_ms.
    const auto turns = tl.turns();
    for (size_t i = 1; i < turns.size(); ++i)
        EXPECT_LE(turns[i - 1].start_ms, turns[i].start_ms);

    // Sample coverage at 50 ms stride across the meeting.
    auto covering = [&](uint64_t ms) {
        int best = -1; float bc = 0.f; int count = 0;
        for (const auto& iv : intervals) {
            if (ms >= iv.s && ms < iv.e) {
                ++count;
                if (iv.conf > bc) { bc = iv.conf; best = iv.spk; }
            }
        }
        return std::pair<int, int>{best, count};
    };
    uint64_t end_ms = 0;
    for (const auto& iv : intervals) end_ms = std::max(end_ms, iv.e);
    for (uint64_t ms = 25; ms < end_ms; ms += 50) {
        auto [expect_spk, expect_n] = covering(ms);
        EXPECT_EQ(tl.activeSpeakerCountAt(ms), expect_n) << "at " << ms;
        int got = tl.dominantSpeakerAt(ms);
        if (expect_n == 0)      EXPECT_EQ(got, -1) << "at " << ms;
        else                    EXPECT_EQ(got, expect_spk) << "at " << ms;
    }

    // Per-interval attribution: three words inside each interval's solo
    // (non-overlapped) part must attribute to that interval's speaker.
    SpeakerAttributor att({0});
    for (const auto& iv : intervals) {
        // find the max-confidence coverer inside this interval's span
        uint64_t probe = (iv.s + iv.e) / 2;
        auto [spk, n] = covering(probe);
        if (n != 1) continue;  // overlapped midpoint — checked separately
        std::vector<Word> ws(3);
        for (auto& w : ws) {
            w.text = "w";
            w.start_ms = static_cast<uint32_t>(probe - 150);
            w.end_ms = static_cast<uint32_t>(probe + 150);
            w.word_is_final = true;
        }
        auto segs = att.attribute(ws, false, tl);
        ASSERT_EQ(segs.size(), 1u) << "interval " << iv.s << "-" << iv.e;
        EXPECT_EQ(segs[0].speaker, spk);
    }

    // A word in verified silence must be unknown.
    for (uint64_t ms = 50; ms < end_ms; ms += 1000) {
        if (covering(ms).second == 0) {
            Word w;
            w.text = "x";
            w.start_ms = static_cast<uint32_t>(ms - 40);
            w.end_ms = static_cast<uint32_t>(ms + 40);
            w.word_is_final = true;
            auto segs = att.attribute({w}, false, tl);
            ASSERT_EQ(segs.size(), 1u);
            EXPECT_EQ(segs[0].speaker, -1) << "silence at " << ms;
            break;  // one silence probe per meeting is enough
        }
    }
}

INSTANTIATE_TEST_SUITE_P(GeneratedMeetings, TurnBattery,
                         ::testing::Range(0, 80));
