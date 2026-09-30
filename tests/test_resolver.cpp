#include <gtest/gtest.h>
#include <cmath>
#include <random>
#include "diar/GlobalSpeakerResolver.h"

using namespace ss;

namespace {

// Deterministic fake embeddings: gaussian random directions per seed
// (well-separated voices => pairwise cosine ~0).
std::vector<float> emb(int seed) {
    std::mt19937 rng{uint32_t(seed)};
    std::normal_distribution<float> d(0.f, 1.f);
    std::vector<float> v(64);
    for (auto& x : v) x = d(rng);
    return v;
}

std::vector<float> noisy(const std::vector<float>& v, float eps = 0.02f) {
    auto out = v;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] += std::cos(float(i) * 3.7f) * eps;
    return out;
}

SpeakerTurn turn(int ch, uint64_t a, uint64_t b) {
    return SpeakerTurn{ch, a, b, 0.9f};
}

} // namespace

TEST(GlobalSpeakerResolver, SameVoiceSamePerson) {
    GlobalSpeakerResolver r;
    int p1 = r.resolve(turn(0, 0, 2000), emb(1));
    int p2 = r.resolve(turn(0, 5000, 8000), noisy(emb(1)));
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(r.personCount(), 1);
}

TEST(GlobalSpeakerResolver, DistinctVoicesDistinctPersons) {
    GlobalSpeakerResolver r;
    EXPECT_EQ(r.resolve(turn(0, 0, 2000), emb(1)), 0);
    EXPECT_EQ(r.resolve(turn(1, 0, 2000), emb(2)), 1);
    EXPECT_EQ(r.resolve(turn(2, 0, 2000), emb(3)), 2);
    EXPECT_EQ(r.personCount(), 3);
}

TEST(GlobalSpeakerResolver, ExceedsChannelLimit) {
    // The whole point: 10 distinct voices on only 3 recycled channels.
    GlobalSpeakerResolver r;
    for (int person = 0; person < 10; ++person) {
        int ch = person % 3;
        int g = r.resolve(turn(ch, person * 5000, person * 5000 + 2000),
                          emb(person + 1));
        EXPECT_EQ(g, person) << "voice " << person << " on channel " << ch;
    }
    EXPECT_EQ(r.personCount(), 10);
}

TEST(GlobalSpeakerResolver, ChannelReusedByNewVoiceGetsNewPerson) {
    GlobalSpeakerResolver r;
    int p1 = r.resolve(turn(0, 0, 2000), emb(42));
    int p2 = r.resolve(turn(0, 10000, 12000), emb(77)); // same channel, new voice
    EXPECT_NE(p1, p2);
    EXPECT_EQ(r.globalForChannel(0), p2); // channel now maps to latest
}

TEST(GlobalSpeakerResolver, ShortTurnsNotResolved) {
    GlobalSpeakerResolver r;
    r.resolve(turn(0, 0, 2000), emb(1));
    // A 300 ms blip on channel 1 shouldn't spawn a new person
    int g = r.resolve(turn(1, 3000, 3300), emb(99));
    EXPECT_EQ(g, -1); // channel 1 has no global mapping yet
    EXPECT_EQ(r.personCount(), 1);
}
