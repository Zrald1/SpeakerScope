// Headless diarization benchmark.
//   speakerscope_diar_bench <wav> [model.gguf]
// Decodes the wav and pushes it through NemotronDiarizer as fast as the
// CPU allows (no realtime pacing, no AssemblyAI). Prints throughput as a
// multiple of realtime plus every speaker turn the model emitted — use it
// to tune SS_DIAR_PROFILE / SS_DIAR_CHUNK_LEN / SS_DIAR_RIGHT_CONTEXT.
//
//   set SS_DIAR_PROFILE=very_high
//   set SS_DIAR_PROFILE=custom & set SS_DIAR_CHUNK_LEN=40 & set SS_DIAR_RIGHT_CONTEXT=4

#include <chrono>
#include <cstdio>
#include <vector>

#include <miniaudio.h>

#include "core/Logger.h"
#include "core/Types.h"
#include "diar/NemotronDiarizer.h"

using namespace ss;

int main(int argc, char** argv) {
    initLogger();
    std::string wav = argc > 1 ? argv[1] : "assets/samples/meeting.wav";
    std::string model =
        argc > 2 ? argv[2] : "models/nemotron-3-diarization-q8_0.gguf";

    ma_decoder_config dcfg =
        ma_decoder_config_init(ma_format_s16, kChannels, kSampleRate);
    ma_decoder dec;
    if (ma_decoder_init_file(wav.c_str(), &dcfg, &dec) != MA_SUCCESS) {
        std::fprintf(stderr, "cannot decode %s\n", wav.c_str());
        return 1;
    }
    std::vector<int16_t> pcm;
    std::vector<int16_t> buf(kSampleRate);
    ma_uint64 got = 0;
    while (ma_decoder_read_pcm_frames(&dec, buf.data(), kSampleRate, &got) ==
               MA_SUCCESS &&
           got > 0)
        pcm.insert(pcm.end(), buf.begin(), buf.begin() + got);
    ma_decoder_uninit(&dec);
    const double audio_s = double(pcm.size()) / kSampleRate;
    std::printf("decoded %s: %.1f s\n", wav.c_str(), audio_s);

    NemotronDiarizer diar(model);
    if (!diar.isReady()) {
        std::fprintf(stderr, "diarizer not ready: %s\n",
                     diar.statusLine().c_str());
        return 2;
    }
    std::printf("diarizer: %s\n", diar.statusLine().c_str());

    std::vector<SpeakerTurn> turns;
    diar.setTurnCallback([&](const SpeakerTurn& t) {
        turns.push_back(t);
    });

    const auto t0 = std::chrono::steady_clock::now();
    const size_t step = kSampleRate / 2; // 500 ms per push
    for (size_t off = 0; off < pcm.size(); off += step) {
        const size_t n = std::min(step, pcm.size() - off);
        diar.pushAudio(pcm.data() + off, n,
                       static_cast<uint64_t>(off) / (kSampleRate / 1000));
    }
    diar.finish();
    const double wall =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0)
            .count();

    std::printf("\n===== RESULT =====\n");
    std::printf("audio: %.1f s   wall: %.1f s   speed: %.2fx realtime\n\n",
                audio_s, wall, audio_s / wall);
    std::printf("turns (%zu):\n", turns.size());
    for (const auto& t : turns)
        std::printf("  speaker_%d  %6.1f - %6.1f s   conf %.2f\n", t.speaker,
                    t.start_ms / 1000.0, t.end_ms / 1000.0, t.confidence);
    return 0;
}
