# Architecture

## Data flow

```
miniaudio capture (mic | WASAPI loopback)  -- audio callback thread
        │ s16le mono 16 kHz
        ▼
AudioRingBuffer (SPSC, lock-free)
        │
   sender thread: drains 50 ms chunks ─┬─► AssemblyAI v3 WS (binary frames)
                                      │        └► Turn{words[{text,start,end}]}
                                      └─► DiarizationEngine::pushAudio
                                               └► ActivityFrame [t×8 probs, 10 ms]
        ▼
SpeakerTimeline (prob frames + materialized turns)
        ▼
SpeakerAttributor (word midpoint argmax + hysteresis)
        ▼
UI (ImGui): TranscriptView + SpeakerLanes
```

## Threading

| Thread | Role | Rules |
|---|---|---|
| audio cb | capture → ring | no alloc, no locks beyond SPSC push |
| sender | ring → WS + diar | owns chunking, timestamps |
| WS cb | parse JSON → TranscriptBuffer | mutex-guarded writes only |
| UI | fusion read model + render | reads via snapshot() |

## Timing contract

Every sample carries a monotonic index from capture start. `ms = idx / 16`.
Both consumers see identical stream-time, so word timestamps (AssemblyAI, ms
relative to stream start) and ActivityFrame.start_ms align directly.
