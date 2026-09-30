#include <gtest/gtest.h>
#include "align/SpeakerAttributor.h"

using namespace ss;

namespace {

std::vector<Word> makeWords(std::initializer_list<const char*> texts,
                            uint32_t start = 0, uint32_t step = 500) {
    std::vector<Word> w;
    uint32_t t = start;
    for (auto* s : texts) {
        w.push_back({s, t, t + 400, 0.99f, true});
        t += step;
    }
    return w;
}

void feed(SpeakerTimeline& tl, uint64_t t0, uint64_t t1, int ch) {
    for (uint64_t t = t0; t < t1; t += 10) {
        ActivityFrame f;
        f.start_ms = t;
        f.probs.fill(0.f);
        f.probs[ch] = 0.9f;
        tl.addFrame(f);
    }
}

} // namespace

TEST(SpeakerAttributor, AssignsWordsToSpeakers) {
    SpeakerTimeline tl;
    feed(tl, 0, 1200, 0);      // speaker 0 talks 0-1.2s
    feed(tl, 1200, 3000, 1);   // speaker 1 talks 1.2-3s

    auto words = makeWords({"hello", "there", "hi", "back"});
    SpeakerAttributor attr;
    auto segs = attr.attribute(words, false, tl);

    ASSERT_GE(segs.size(), 2u);
    EXPECT_EQ(segs.front().speaker, 0);
    EXPECT_EQ(segs.back().speaker, 1);
    EXPECT_EQ(segs.front().text, "hello there");
    EXPECT_EQ(segs.back().text, "hi back");
}

TEST(SpeakerAttributor, UnknownWhenNoDiarization) {
    SpeakerTimeline tl; // empty
    auto words = makeWords({"a", "b"});
    SpeakerAttributor attr;
    auto segs = attr.attribute(words, false, tl);
    ASSERT_EQ(segs.size(), 1u);
    EXPECT_EQ(segs[0].speaker, -1);
}

TEST(SpeakerAttributor, FlagsOverlap) {
    SpeakerTimeline tl;
    for (uint64_t t = 0; t < 2000; t += 10) {
        ActivityFrame f;
        f.start_ms = t;
        f.probs.fill(0.f);
        f.probs[0] = 0.9f;
        if (t >= 400 && t < 900) f.probs[1] = 0.85f; // crosstalk
        tl.addFrame(f);
    }
    auto words = makeWords({"one", "two", "three", "four"}, 0, 500);
    SpeakerAttributor attr;
    auto segs = attr.attribute(words, false, tl);
    bool any_overlap = false;
    for (auto& s : segs) any_overlap |= s.overlap;
    EXPECT_TRUE(any_overlap);
}
