#include <gtest/gtest.h>
#include "stt/StreamingMessages.h"
#include "stt/TranscriptBuffer.h"

using namespace ss;

TEST(TranscriptBuffer, PartialThenFinal) {
    TranscriptBuffer tb;
    Turn t;
    t.turn_order = 0;
    t.transcript = "hel";
    t.end_of_turn = false;
    tb.onTurn(t);
    EXPECT_TRUE(tb.finalizedTurns().empty());
    EXPECT_EQ(tb.partialTurns().size(), 1u);

    t.transcript = "hello world";
    t.end_of_turn = true;
    tb.onTurn(t);
    EXPECT_EQ(tb.finalizedTurns().size(), 1u);
    EXPECT_EQ(tb.finalizedText(), "hello world");
    EXPECT_TRUE(tb.partialTurns().empty());
}

TEST(StreamingMessages, ParsesTurnWithWords) {
    const char* frame = R"({
        "type": "Turn", "turn_order": 3, "transcript": "test phrase",
        "end_of_turn": true, "end_of_turn_confidence": 0.92,
        "words": [
            {"start": 100, "end": 300, "text": "test", "confidence": 0.98},
            {"start": 320, "end": 700, "text": "phrase", "confidence": 0.91}
        ]
    })";
    auto ev = parseServerMessage(frame);
    EXPECT_EQ(ev.type, ServerMsgType::Turn);
    EXPECT_TRUE(ev.turn.end_of_turn);
    ASSERT_EQ(ev.turn.words.size(), 2u);
    EXPECT_EQ(ev.turn.words[0].text, "test");
    EXPECT_EQ(ev.turn.words[1].end_ms, 700u);
}

TEST(StreamingMessages, ParsesBegin) {
    auto ev = parseServerMessage(
        R"({"type":"Begin","id":"abc123","expires_at":1750000000})");
    EXPECT_EQ(ev.type, ServerMsgType::Begin);
    EXPECT_EQ(ev.session_id, "abc123");
}

TEST(StreamingMessages, BadJsonIsUnknown) {
    auto ev = parseServerMessage("not json at all {");
    EXPECT_EQ(ev.type, ServerMsgType::Unknown);
}
