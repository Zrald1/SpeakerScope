// Turn-path coverage: SpeakerTimeline::addTurn + turn-based query fallback
// (the path NemotronDiarizer/audio.cpp drives), plus SpeakerRegistry.
#include <gtest/gtest.h>
#include "align/SpeakerAttributor.h"
#include "diar/SpeakerRegistry.h"
#include "diar/SpeakerTimeline.h"

using namespace ss;

namespace {

SpeakerTurn turn(int spk, uint64_t s, uint64_t e, float c = 0.9f) {
    SpeakerTurn t;
    t.speaker = spk;
    t.start_ms = s;
    t.end_ms = e;
    t.confidence = c;
    return t;
}

Word word(uint64_t s, uint64_t e, const char* t = "w") {
    Word w;
    w.text = t;
    w.start_ms = static_cast<uint32_t>(s);
    w.end_ms = static_cast<uint32_t>(e);
    w.confidence = 0.9f;
    w.word_is_final = true;
    return w;
}

} // namespace

// ---- addTurn basics -------------------------------------------------------

TEST(TurnPath, DominantSpeakerFromTurn) {
    SpeakerTimeline tl;
    tl.addTurn(turn(2, 1000, 3000));
    EXPECT_EQ(tl.dominantSpeakerAt(2000), 2);
    EXPECT_EQ(tl.dominantSpeakerAt(999), -1);   // before turn
    EXPECT_EQ(tl.dominantSpeakerAt(3000), -1);  // end exclusive
}

TEST(TurnPath, TurnListIsSortedAfterLateInsert) {
    SpeakerTimeline tl;
    tl.addTurn(turn(0, 5000, 6000));  // arrives first (closed later)
    tl.addTurn(turn(1, 1000, 2000));  // concurrent speaker closes second
    auto turns = tl.turns();
    ASSERT_EQ(turns.size(), 2u);
    EXPECT_EQ(turns[0].speaker, 1);
    EXPECT_EQ(turns[1].speaker, 0);
    // and queries resolve the earlier turn even though it arrived last
    EXPECT_EQ(tl.dominantSpeakerAt(1500), 1);
}

TEST(TurnPath, OverlapCountFromTurns) {
    SpeakerTimeline tl;
    tl.addTurn(turn(0, 0, 4000));
    tl.addTurn(turn(1, 2000, 5000));
    EXPECT_EQ(tl.activeSpeakerCountAt(1000), 1);
    EXPECT_EQ(tl.activeSpeakerCountAt(3000), 2);
    EXPECT_EQ(tl.activeSpeakerCountAt(4500), 1);
    EXPECT_EQ(tl.activeSpeakerCountAt(6000), 0);
}

TEST(TurnPath, OverlappingTurnsPickHigherConfidence) {
    SpeakerTimeline tl;
    tl.addTurn(turn(0, 0, 4000, 0.6f));
    tl.addTurn(turn(1, 0, 4000, 0.9f));
    EXPECT_EQ(tl.dominantSpeakerAt(2000), 1);
}

TEST(TurnPath, DropSubMinTurnBlips) {
    SpeakerTimeline tl;  // min_turn_ms = 250
    tl.addTurn(turn(0, 0, 100));   // 100 ms blip -> dropped
    tl.addTurn(turn(-1, 0, 1000)); // invalid channel -> dropped
    tl.addTurn(turn(0, 500, 500)); // zero-length -> dropped
    EXPECT_TRUE(tl.turns().empty());
    EXPECT_EQ(tl.dominantSpeakerAt(700), -1);
}

TEST(TurnPath, OpenTurnExtensionMerges) {
    SpeakerTimeline tl;
    // audio.cpp re-emits an open turn each window with a longer end.
    tl.addTurn(turn(0, 1000, 2000));
    tl.addTurn(turn(0, 1000, 3500));
    auto turns = tl.turns();
    ASSERT_EQ(turns.size(), 1u);
    EXPECT_EQ(turns[0].start_ms, 1000u);
    EXPECT_EQ(turns[0].end_ms, 3500u);
    EXPECT_EQ(tl.dominantSpeakerAt(3400), 0);
}

TEST(TurnPath, SameSpeakerWithinGapMerges) {
    SpeakerTimeline tl;  // merge_gap_ms = 300
    tl.addTurn(turn(3, 0, 1000));
    tl.addTurn(turn(3, 1200, 2500));  // 200 ms gap -> merge
    auto turns = tl.turns();
    ASSERT_EQ(turns.size(), 1u);
    EXPECT_EQ(turns[0].end_ms, 2500u);
}

TEST(TurnPath, SameSpeakerBeyondGapDoesNotMerge) {
    SpeakerTimeline tl;
    tl.addTurn(turn(3, 0, 1000));
    tl.addTurn(turn(3, 2000, 3000));  // 1000 ms gap -> separate
    EXPECT_EQ(tl.turns().size(), 2u);
}

TEST(TurnPath, DifferentSpeakersNeverMerge) {
    SpeakerTimeline tl;
    tl.addTurn(turn(0, 0, 1000));
    tl.addTurn(turn(1, 1000, 2000));  // touching but different channel
    EXPECT_EQ(tl.turns().size(), 2u);
}

TEST(TurnPath, MixedFrameAndTurnFeeds) {
    SpeakerTimeline tl;
    tl.addTurn(turn(5, 500, 1500));
    // frames elsewhere don't disturb the turn fallback
    for (uint64_t t = 5000; t < 6000; t += 10) {
        ActivityFrame f;
        f.start_ms = t;
        f.probs[1] = 0.9f;
        tl.addFrame(f);
    }
    EXPECT_EQ(tl.dominantSpeakerAt(1000), 5);
    EXPECT_EQ(tl.dominantSpeakerAt(5500), 1);
}

TEST(TurnPath, ClearResetsTurns) {
    SpeakerTimeline tl;
    tl.addTurn(turn(0, 0, 9000));
    tl.clear();
    EXPECT_TRUE(tl.turns().empty());
    EXPECT_EQ(tl.dominantSpeakerAt(4500), -1);
    EXPECT_EQ(tl.activeSpeakerCountAt(4500), 0);
}

// ---- Attributor over the turn path ---------------------------------------

TEST(TurnPathAttribution, WordsInsideTurnGetSpeaker) {
    SpeakerTimeline tl;
    tl.addTurn(turn(1, 0, 10000));
    SpeakerAttributor att;
    std::vector<Word> ws = {word(0, 400), word(500, 900), word(1000, 1400)};
    auto segs = att.attribute(ws, false, tl);
    ASSERT_EQ(segs.size(), 1u);
    EXPECT_EQ(segs[0].speaker, 1);
    EXPECT_EQ(segs[0].text, "w w w");
}

TEST(TurnPathAttribution, WordOutsideTurnIsUnknown) {
    SpeakerTimeline tl;
    tl.addTurn(turn(0, 2000, 5000));
    SpeakerAttributor att({0});
    auto segs = att.attribute({word(0, 300)}, false, tl);
    ASSERT_EQ(segs.size(), 1u);
    EXPECT_EQ(segs[0].speaker, -1);
}

TEST(TurnPathAttribution, OverlappingTurnsFlagSegment) {
    SpeakerTimeline tl;
    tl.addTurn(turn(0, 0, 5000, 0.9f));
    tl.addTurn(turn(1, 0, 5000, 0.6f));
    SpeakerAttributor att;
    auto segs = att.attribute({word(1000, 1400)}, false, tl);
    ASSERT_EQ(segs.size(), 1u);
    EXPECT_TRUE(segs[0].overlap);
    EXPECT_EQ(segs[0].speaker, 0); // higher confidence wins
}

TEST(TurnPathAttribution, SpeakerChangeMidTurnSplitsSegment) {
    SpeakerTimeline tl;
    tl.addTurn(turn(0, 0, 2000));
    tl.addTurn(turn(1, 2000, 4000));
    SpeakerAttributor att({0}); // no hysteresis
    std::vector<Word> ws = {word(200, 600), word(800, 1200),
                            word(2200, 2600), word(2800, 3200)};
    auto segs = att.attribute(ws, false, tl);
    ASSERT_EQ(segs.size(), 2u);
    EXPECT_EQ(segs[0].speaker, 0);
    EXPECT_EQ(segs[1].speaker, 1);
}

TEST(TurnPathAttribution, LateArrivingTurnReAttributesWords) {
    // Words are re-attributed on every snapshot — a turn that lands late
    // (closed after the words arrived) must upgrade [?] to a speaker.
    SpeakerTimeline tl;
    SpeakerAttributor att({0});
    std::vector<Word> ws = {word(1000, 1400), word(1500, 1900)};
    EXPECT_EQ(att.attribute(ws, false, tl)[0].speaker, -1);
    tl.addTurn(turn(4, 500, 2500));  // arrives after the words
    auto segs = att.attribute(ws, false, tl);
    ASSERT_EQ(segs.size(), 1u);
    EXPECT_EQ(segs[0].speaker, 4);
}

// ---- SpeakerRegistry ------------------------------------------------------

TEST(Registry, NoteActiveMarksSeen) {
    SpeakerRegistry r;
    EXPECT_FALSE(r.seen(0));
    EXPECT_EQ(r.speakerCount(), 0);
    r.noteActive(0);
    r.noteActive(3);
    EXPECT_TRUE(r.seen(0));
    EXPECT_TRUE(r.seen(3));
    EXPECT_FALSE(r.seen(1));
    EXPECT_EQ(r.speakerCount(), 2);
}

TEST(Registry, NoteActiveIgnoresOutOfRange) {
    SpeakerRegistry r;
    r.noteActive(-1);
    r.noteActive(kMaxSpeakers);
    r.noteActive(1000);
    EXPECT_EQ(r.speakerCount(), 0);
}

TEST(Registry, RenameToEmptyKeepsDefault) {
    SpeakerRegistry r;
    r.rename(0, "");
    EXPECT_EQ(r.name(0), "Person 1");
    r.rename(0, "Zaira");
    EXPECT_EQ(r.name(0), "Zaira");
}

TEST(Registry, OutOfRangeNameIsUnknown) {
    SpeakerRegistry r;
    EXPECT_EQ(r.name(-1), "Unknown");
    EXPECT_EQ(r.name(kMaxSpeakers), "Unknown");
}

TEST(Registry, ColorsAreDistinct) {
    SpeakerRegistry r;
    for (int a = 0; a < kMaxSpeakers; ++a)
        for (int b = a + 1; b < kMaxSpeakers; ++b)
            EXPECT_NE(r.color(a), r.color(b)) << a << " vs " << b;
}
