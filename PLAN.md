# SpeakerScope — Hackathon Master Plan

**Hackathon:** lablab.ai × AssemblyAI — "Build voice AI agents on AssemblyAI" (Sept 1–30, 2026)
**Path chosen:** Realtime Speech-to-Text API (bring your own orchestration), with an optional Voice Agent API stretch goal.
**Language:** C++ (C++20), Windows-first desktop app, CMake + vcpkg/FetchContent.

---

## 1. Product vision

**SpeakerScope** is a Windows desktop app that listens to live audio — your microphone **or** system/loopback audio (Zoom, Meet, Teams, podcasts, videos) — and produces a **live, speaker-attributed transcript**:

- **NVIDIA Nemotron-3-Diarization** runs **fully on-device** (native C++, GGUF weights, no Python) and continuously answers *"who is speaking when"* — up to **8 speakers**, including **overlapping speech**.
- **AssemblyAI Universal Streaming v3** answers *"what is being said"* with sub-second, word-timestamped transcripts.
- A custom **alignment engine** fuses the two streams by timestamp, producing a color-coded, per-speaker transcript in real time.
- Stretch: an in-app **voice agent** (AssemblyAI Voice Agent API or LLM Gateway) you can ask: *"What did speaker 2 decide?"* — live meeting Q&A.

### Why this can win (mapped to judging criteria)

| Criterion | Our angle |
|---|---|
| Application of Technology | Novel hybrid stack nobody else will submit: cloud SOTA STT (AssemblyAI) + on-device SOTA diarization (Nemotron-3, #1 on VoiceArena Diarization-Bench, 14.72% DER). Timestamp-level fusion is real engineering, not an API wrapper. |
| Business Value | Meeting transcription, call-center QA, podcast indexing, accessibility. Privacy story: speaker separation never leaves the machine; only audio-for-transcription goes to AssemblyAI. |
| Originality | Diarization + streaming ASR fusion in a native C++ desktop app — essentially zero hackathon boilerplate exists for this. Overlap-aware, 8-speaker support is beyond typical "speaker_labels" demos. |
| Presentation | Live demo: play a 4-person podcast clip through loopback → watch color-coded speaker transcript build in real time. Instantly legible to judges. |

### Name/branding
Working title **SpeakerScope**. Tagline: *"Every voice, attributed."*

---

## 2. Architecture

```
            ┌──────────────────────────────────────────────────────────┐
            │                     Audio Sources                        │
            │   Microphone (WASAPI)      System Loopback (WASAPI)      │
            └───────────────┬──────────────────────┬───────────────────┘
                            │                      │
                            ▼                      ▼
                    ┌───────────────────────────────────┐
                    │  AudioCapture (miniaudio)         │
                    │  → Resampler → 16 kHz mono PCM    │
                    │  → AudioRingBuffer (lock-free)    │
                    └───────┬───────────────┬───────────┘
                            │               │
              same PCM stream tees to BOTH  │               │
                            │               │
                            ▼               ▼
        ┌────────────────────────┐  ┌─────────────────────────┐
        │ NemotronDiarizer       │  │ AssemblyAIClient        │
        │ (NeMo-Speech.cpp /     │  │ (IXWebSocket →          │
        │  audio.cpp, GGUF,      │  │  wss://streaming.       │
        │  streaming mode)       │  │  assemblyai.com/v3/ws)  │
        │                        │  │  pcm_s16le @16kHz,      │
        │ OUT: speaker activity  │  │  ~50ms binary frames    │
        │ timeline [t × 8 spk]   │  │                         │
        │ → SpeakerTurn{start,   │  │ OUT: Turn events        │
        │   end, speaker_id}     │  │   words[{text,start,end}]│
        └───────────┬────────────┘  └────────────┬────────────┘
                    │                            │
                    ▼                            ▼
        ┌─────────────────────────────────────────────────┐
        │ SpeakerAttributor (alignment engine)            │
        │ For each word: argmax speaker prob at word mid- │
        │ point → AttributedWord{text, speaker, t}        │
        │ Smoothing: majority-vote over turn, hysteresis  │
        └───────────────────────┬─────────────────────────┘
                                ▼
        ┌─────────────────────────────────────────────────┐
        │ UI (Dear ImGui + GLFW, or Qt6 — see §4)         │
        │ • Live color-coded transcript per speaker       │
        │ • Speaker activity lanes (8-channel ribbon)     │
        │ • Speaker count / overlap indicator             │
        │ • Export: .txt / .srt / .json                   │
        └─────────────────────────────────────────────────┘
```

**Key design decision — tee the audio, don't serialize it.** Both engines consume the *same* 16 kHz PCM stream independently. Diarization and ASR have different latencies; the ring buffer + monotonic sample clock keeps them time-aligned. Every audio sample carries a `sample_index`; all timestamps derive from it.

---

## 3. Tech stack

| Layer | Choice | Why | Fallback |
|---|---|---|---|
| Build | CMake 3.26+, MSVC 2022, vcpkg manifest | Standard Windows C++ | FetchContent-only |
| Audio capture | **miniaudio** (single header) | Mic + WASAPI **loopback** in one lib, tiny | PortAudio/RtAudio |
| Diarization runtime | **NeMo-Speech.cpp** (official NVIDIA C++ ggml runtime) — `nvidia/nemotron` GGUF family | Pure C++, official, streaming mode, CPU+CUDA | `audio.cpp` (community, `audio-cpp/Nemotron-3-Diarization-GGUF`) → last resort: Python NeMo sidecar over localhost gRPC |
| Diarization weights | `nemotron-3-diarization-bf16.gguf` (189 MB) or `-q8_0.gguf` (102 MB) | OpenMDW-1.1 license, redistributable | `.nemo` via NeMo sidecar |
| WebSocket | **IXWebSocket** | Header-lib-quality, TLS, binary frames, ping/pong | Boost.Beast |
| JSON | nlohmann/json | Standard | — |
| HTTP (token fetch) | cpr or cpp-httplib | Temp auth tokens | — |
| UI | **Dear ImGui + GLFW + OpenGL3** | Compiles in seconds, custom speaker-lane ribbon widget, easy animated transcript | **Qt6 Widgets** if we want native-polish (decision point §4) |
| Logging | spdlog | Standard | — |
| Config | TOML (toml++) or JSON | API key via env/`config.toml` (never committed) | — |

**AssemblyAI session params** (from v3 spec): `sample_rate=16000`, `encoding=pcm_s16le`, `speech_model=u3-rt-pro` (voice-optimized) or `universal-3-5-pro`, `format_turns=true`, `min_turn_silence`/`max_turn_silence` tuning, `inactivity_timeout`. Auth via `Authorization` header or `token` query param (temporary token endpoint). **Always send `Terminate`** — billing runs while the socket is open.

**Nemotron streaming config:** latency profile ≈ 1.04 s (recommended operating point, ~40% relative DER reduction vs Sortformer baseline); `CHUNK_LEN`, `FIFO_LEN`, `SPKCACHE_LEN`, `RIGHT_CONTEXT` exposed via runtime session options; output frame stride 10 ms × 8 channels.

---

## 4. Decision points (need your call before coding)

1. **UI toolkit — Dear ImGui (recommended) vs Qt6.** ImGui = faster to build, single exe, gamer-style look; Qt6 = native polish, heavier install. *Recommendation: ImGui for the deadline.*
2. **Diarization backend — NeMo-Speech.cpp (recommended) vs audio.cpp vs Python sidecar.** Build the `DiarizationEngine` interface so backends are swappable. *Recommendation: try NeMo-Speech.cpp first, audio.cpp second; keep a `WavFileDiarizer` test double regardless.*
3. **Voice-agent stretch goal** — only after MVP ships (see §7).
4. **Interpretation check:** I read "multiple OJUMP" as **multiple speakers / people jumping into a conversation** — i.e., multi-speaker detection incl. overlaps. Correct me if you meant something else.

---

## 5. Repository layout (files & folders, fully designated)

```
hackathonforassembly/
├── PLAN.md                          ← this file
├── README.md                        ← judges' entry point (screenshots, setup, demo gif)
├── LICENSE                          ← MIT (required by hackathon rules)
├── CMakeLists.txt                   ← root project
├── vcpkg.json                       ← manifest: ixwebsocket, nlohmann-json, glfw3, glad, imgui, spdlog, cpr, tomlplusplus, gtest
├── .gitignore                       ← models/, build/, config.toml, .env
├── .env.example                     ← ASSEMBLYAI_API_KEY=...
│
├── cmake/
│   ├── FetchMiniaudio.cmake         ← header-only, fetched not vendored
│   └── Warnings.cmake               ← /W4, treat-warnings-as-errors target
│
├── third_party/
│   └── README.md                    ← note: deps via vcpkg; nothing vendored
│
├── models/                          ← GITIGNORED, downloaded by script
│   └── nemotron-3-diarization-bf16.gguf
│
├── assets/
│   ├── fonts/Inter-Regular.ttf      ← UI font (OFL license)
│   ├── icon.ico
│   └── samples/                     ← test wavs: 2-speaker, overlap, 4-speaker podcast clip
│
├── src/
│   ├── main.cpp                     ← entry: parse args, init app, run UI loop
│   │
│   ├── core/
│   │   ├── Config.h / Config.cpp                ← env + toml config, API key load
│   │   ├── Logger.h / Logger.cpp                ← spdlog init, rotating file sink
│   │   ├── Types.h                              ← SpeakerTurn, Word, AttributedWord, AudioChunk{data,sample_rate,sample_index}
│   │   └── Clock.h                              ← monotonic sample clock ↔ wall clock mapping
│   │
│   ├── audio/
│   │   ├── AudioCapture.h / .cpp                ← miniaudio device wrapper (mic)
│   │   ├── LoopbackCapture.h / .cpp             ← WASAPI loopback (system audio)
│   │   ├── AudioSource.h                        ← interface: start/stop/callback
│   │   ├── Resampler.h / .cpp                   ← any-rate → 16 kHz mono s16le
│   │   └── AudioRingBuffer.h                    ← lock-free SPSC ring, 30 s capacity
│   │
│   ├── diar/
│   │   ├── DiarizationEngine.h                  ← interface: pushPcm(), onSpeakerActivity(cb)
│   │   ├── NemotronDiarizer.h / .cpp            ← NeMo-Speech.cpp / audio.cpp binding (streaming session)
│   │   ├── WavFileDiarizer.h / .cpp             ← test double: replays gold RTTM for tests/demo w/o model
│   │   ├── SpeakerTimeline.h / .cpp             ← merged [t×8] probs → debounced SpeakerTurn list
│   │   └── SpeakerRegistry.h / .cpp             ← ch0..ch7 → "Speaker 1..8", colors, rename to names
│   │
│   ├── stt/
│   │   ├── AssemblyAIClient.h / .cpp            ← IXWebSocket lifecycle, reconnect w/ backoff, send Terminate
│   │   ├── StreamingMessages.h / .cpp           ← Begin/Turn/SpeechStarted/Termination (de)serialization
│   │   ├── TokenProvider.h / .cpp               ← temporary-token endpoint (so key isn't in WS URL)
│   │   └── TranscriptBuffer.h / .cpp            ← partial vs end_of_turn handling, word store
│   │
│   ├── align/
│   │   ├── SpeakerAttributor.h / .cpp           ← word→speaker alignment (midpoint argmax + hysteresis)
│   │   ├── AttributedTranscript.h / .cpp        ← ordered doc model, revision handling (Turn updates)
│   │   └── OverlapAnnotator.h / .cpp            ← marks segments where ≥2 channels active ("crosstalk")
│   │
│   ├── agent/                                   ← STRETCH GOAL
│   │   ├── VoiceAgentClient.h / .cpp            ← AssemblyAI Voice Agent API session
│   │   └── MeetingQa.h / .cpp                   ← "ask the transcript" tool-call: JSON-schema tool
│   │
│   ├── ui/
│   │   ├── AppWindow.h / .cpp                   ← GLFW + ImGui frame, docking layout
│   │   ├── TranscriptView.h / .cpp              ← scrolling attributed transcript, per-speaker colors
│   │   ├── SpeakerLanes.h / .cpp                ← custom widget: 8 horizontal activity ribbons + overlap glow
│   │   ├── DevicePicker.h / .cpp                ← mic vs loopback source selector
│   │   ├── ExportDialog.h / .cpp                ← save .txt/.srt/.json
│   │   └── Theme.h                              ← speaker color palette (8 distinct, colorblind-safe)
│   │
│   └── app/
│       ├── Application.h / .cpp                 ← owns all subsystems, main loop
│       └── SessionController.h / .cpp           ← start/stop session, thread orchestration, state machine
│
├── tests/
│   ├── CMakeLists.txt                           ← gtest
│   ├── test_resampler.cpp
│   ├── test_speaker_timeline.cpp                ← prob matrix → turns, debounce edges
│   ├── test_attributor.cpp                      ← alignment on synthetic overlaps
│   └── fixtures/                                ← pcm fixtures, gold rttm, gold transcripts
│
├── scripts/
│   ├── setup.ps1                                ← vcpkg bootstrap, cmake configure, build
│   ├── download_models.ps1                      ← hf download GGUF weights → models/
│   ├── download_models.sh                       ← same for CI/linux
│   └── package.ps1                              ← zip release: exe + assets + models downloader
│
├── docs/
│   ├── ARCHITECTURE.md                          ← diagrams, data flow, threading model
│   ├── DEMO_SCRIPT.md                           ← exact steps for video recording
│   ├── SUBMISSION.md                            ← lablab fields: short/long desc, tags, links
│   └── slides/                                  ← presentation deck
│
└── .github/
    └── workflows/build.yml                      ← CI: build + tests on windows-latest
```

### Threading model (4 threads, clean boundaries)

| Thread | Work |
|---|---|
| Audio callback | miniaudio cb → resample → push to ring buffer. *No allocation, no locks beyond SPSC.* |
| Diarization worker | pops chunks (e.g., 124 ms steps), feeds Nemotron session, emits activity probs |
| Network worker | pops ~50 ms frames → WS binary send; parse incoming JSON on IXWebSocket callback thread → queue |
| UI/main | attributor merge + render at 60 fps; all cross-thread data via SPSC queues |

---

## 6. The alignment engine (the secret sauce)

Nemotron gives `[T, 8]` speaker-activity probabilities at 10 ms resolution. AssemblyAI `Turn` events give `words[{text, start_ms, end_ms}]`.

```
for each word w with midpoint t_w = (w.start + w.end)/2:
    s*(t_w) = argmax_i  P_i[t_w]            // dominant speaker channel
    conf    = P_s*[t_w]
apply hysteresis: speaker change only if new channel dominates for ≥ 200 ms
group consecutive words by speaker → AttributedSegment
flag segments where Σ_i 1[P_i > θ] ≥ 2  → "overlap" styling in UI
```

Handle **partial Turn revisions**: `Turn` messages with `end_of_turn=false` get re-emitted; `AttributedTranscript` stores per-turn state and re-labels on update. (If `speaker_labels=true` is set on AssemblyAI, we'd also get `SpeakerRevision` — we *don't* use it: Nemotron is our speaker authority; that's the differentiator.)

---

## 7. Milestones — MVP-first (deadline is TODAY-adjacent, so order matters)

| # | Milestone | Definition of done |
|---|---|---|
| M0 | **Skeleton builds** | CMake + vcpkg configure; empty ImGui window on screen; CI green |
| M1 | **STT pipeline** | Mic → resample → AssemblyAI v3 → live transcript text in UI (no speakers yet). *This alone is a demoable floor.* |
| M2 | **Loopback capture** | Same pipeline on system audio → transcribe a YouTube podcast live |
| M3 | **Diarization on file** | `audiocpp/nemo-speech` CLI on a wav → turns JSON; `SpeakerTimeline` parses it; `WavFileDiarizer` test double works |
| M4 | **Streaming diarization in-app** | `NemotronDiarizer` live on mic/loopback; SpeakerLanes ribbon shows who-when |
| M5 | **Fusion** | Attributor live: color-coded speaker transcript; **← full MVP demo** |
| M6 | **Polish** | Speaker rename ("Host"/"Guest"), overlap badge, export srt/txt/json, icon, README+gif |
| M7 | **Stretch** | Voice-agent Q&A ("what did Speaker 3 promise?") via Voice Agent API / LLM Gateway w/ transcript as JSON-schema tool |

**Order rationale:** M1+M2 give a working submission even if diarization fights us. M5 is the money shot.

---

## 8. Risk register

| Risk | Mitigation |
|---|---|
| NeMo-Speech.cpp / audio.cpp streaming-diar path immature ("room for optimization" per README) | `DiarizationEngine` interface; fallback = offline chunked diarization on rolling 30 s windows, or Python NeMo sidecar via localhost pipe |
| Nemotron GGUF requires HF token / license click-through | `download_models.ps1` prompts for `HF_TOKEN`; document OpenMDW-1.1 license in README |
| WS protocol drift (v3 fields) | `StreamingMessages` isolated; log raw frames; pin `AssemblyAI-Version` header |
| Timestamp skew between the two consumers | single sample clock; attributor works purely in sample-time domain |
| WASAPI loopback unavailable (some drivers) | mic-only mode degrades gracefully; stereo-mix fallback note |
| GPU-less judge machine | q8_0 GGUF (~102 MB) runs realtime on CPU; bundle CPU build |
| Time (hackathon ends today) | MVP-first ordering; cut M6/M7, not M5 |

---

## 9. Submission checklist (lablab requires ALL of these)

- [ ] Project title + short description + long description (draft in `docs/SUBMISSION.md`)
- [ ] Technology & category tags: `AssemblyAI`, `NVIDIA Nemotron`, `speaker diarization`, `C++`, `real-time`, `desktop`
- [ ] Cover image (screenshot of SpeakerLanes + transcript)
- [ ] Video presentation (2–4 min: problem → live loopback demo on a 4-speaker clip → architecture slide → stretch)
- [ ] Slide deck (`docs/slides/`)
- [ ] Public GitHub repo, MIT license
- [ ] App hosting / application URL — desktop app: GitHub Releases zip counts; also record a hosted demo page or streamable link
- [ ] API credits: signed up via the event link → `ASSEMBLYAI_API_KEY` in `.env` (gitignored)

---

## 10. What I need from you to start building

1. **`ASSEMBLYAI_API_KEY`** — sign up via the hackathon credit link, put it in `.env` locally (never commit).
2. **HF token** (free) for `nvidia/Nemotron-3-Diarization` + `audio-cpp/Nemotron-3-Diarization-GGUF` download — accept the OpenMDW license on the model page.
3. Confirm the §4 decisions (or I'll go with recommendations: **ImGui + NeMo-Speech.cpp**).

Say the word and I'll scaffold the repo (CMake, vcpkg manifest, folder tree, stub modules) and then start on M1.
