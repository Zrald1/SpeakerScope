#include <gtest/gtest.h>
#include "diar/SpeakerTimeline.h"

using namespace ss;

namespace {

ActivityFrame frame(uint64_t ms, int channel, float prob = 0.9f) {
    ActivityFrame f;
    f.start_ms = ms;
    f.probs.fill(0.f);
    if (channel >= 0) f.probs[channel] = prob;
    return f;
}

} // namespace

TEST(SpeakerTimeline, DominantSpeaker) {
    SpeakerTimeline tl;
    for (uint64_t t = 0; t < 1000; t += 10) tl.addFrame(frame(t, 0));
    for (uint64_t t = 1000; t < 2000; t += 10) tl.addFrame(frame(t, 3));
    EXPECT_EQ(tl.dominantSpeakerAt(500), 0);
    EXPECT_EQ(tl.dominantSpeakerAt(1500), 3);
}

TEST(SpeakerTimeline, OverlapCount) {
    SpeakerTimeline tl;
    for (uint64_t t = 0; t < 500; t += 10) {
        ActivityFrame f;
        f.start_ms = t;
        f.probs.fill(0.f);
        f.probs[1] = 0.9f;
        f.probs[2] = 0.8f;
        tl.addFrame(f);
    }
    EXPECT_EQ(tl.activeSpeakerCountAt(250), 2);
}

TEST(SpeakerTimeline, MaterializesTurnsAndMergesGaps) {
    SpeakerTimeline tl;
    // speaker 0: 0-500ms, gap 100ms, 600-1100ms -> merged into one turn
    for (uint64_t t = 0; t < 2000; t += 10) {
        int ch = (t < 500 || (t >= 600 && t < 1100)) ? 0 : -1;
        tl.addFrame(frame(t, ch));
    }
    auto turns = tl.turns();
    ASSERT_EQ(turns.size(), 1u);
    EXPECT_EQ(turns[0].speaker, 0);
    EXPECT_EQ(turns[0].start_ms, 0u);
}

TEST(SpeakerTimeline, DropsShortBlips) {
    SpeakerTimeline tl;
    for (uint64_t t = 0; t < 100; t += 10) tl.addFrame(frame(t, 2)); // 100 ms
    for (uint64_t t = 100; t < 3000; t += 10) tl.addFrame(frame(t, -1));
    EXPECT_TRUE(tl.turns().empty());
}
