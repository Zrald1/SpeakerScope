# Speaker architecture — beyond 8 channels

## Two-level identity model

Nemotron-3-Diarization is **arrival-ordered and window-local**: it tracks at
most 8 *simultaneously distinct* voices, labeled ch0–ch7 by first appearance.
That is a **concurrency** limit, not a total-people limit. Real sessions can
have many more than 8 *distinct* people over time (panel shows, call centers,
classrooms).

We therefore split identity into two levels:

| Level | Source | Meaning | Bound |
|---|---|---|---|
| **Local channel** | Nemotron-3 `ActivityFrame` probs | "which of the ≤8 active voices now" | ≤ 8 concurrent |
| **Global person** | `GlobalSpeakerResolver` (embeddings) | "which human, ever" | unlimited |

`SpeakerRegistry` names/colors remain per global person; the 8 lanes show
*current activity*, the transcript shows *global identity*.

## The trick: turn-level embedding + online clustering

```
SpeakerTimeline emits closed SpeakerTurn{ch, t0, t1}
        │  (t1-t0 >= ~0.7 s)
        ▼
slice PCM[t0:t1] from PcmArchive (rolling session audio)
        ▼
IEmbedder.embed(pcm) -> 192-d vector   (TitaNet-Large GGUF / WeSpeaker ONNX)
        ▼
cosine(emb, centroid_i) for all known persons
        ├─ max >= τ (~0.72): assign existing global person, update centroid
        └─ else:             create new global person "Person N+1"
```

This is the same local-EEND + global-clustering pattern used by pyannote and
NeMo's clustering diarizer — but here the *local* stage is Nemotron's
overlap-aware neural diarization (strictly better than VAD+clustering for
crosstalk), and the *global* stage only runs on clean single-speaker turns.

## Model choices

| Role | Primary | Fallback |
|---|---|---|
| Local diarization | `audio.cpp` `nemotron_3_diar` GGUF (8-spk, streaming) | NeMo-Speech.cpp `sortformer-v2` (4-spk) — note: NeMo-Speech.cpp does **not** yet support Nemotron-3 |
| Global embedding | TitaNet-Large GGUF (`cstr/titanet-large-GGUF`, 192-d, CC-BY-4.0, ~45 MB, 0.66% EER) | WeSpeaker ResNet34 ONNX via onnxruntime (`Alkd/speaker-embedding-onnx`) |

## Correctness rules

- Never trust an embedding from an *overlap* frame — `activeSpeakerCountAt > 1`
  suppresses embedding extraction for that turn.
- Turns < `min_turn_ms` keep their local channel label but don't create
  clusters (short "yeah"s are unreliable embedding sources).
- Centroids are running means capped at N utterances each, so a voice can
  drift (mic change, tone shift) without splitting.
- Local→global mapping is cached per channel-per-window; if the model reuses
  a channel for a new voice (9th speaker), the embedding mismatch creates a
  new person instead of misattributing.

## Status

- [x] `GlobalSpeakerResolver` — online centroid clustering (tested)
- [ ] `TitaNetEmbedder` — GGUF inference binding
- [ ] `OnnxEmbedder` — WeSpeaker ResNet34 via onnxruntime
- [ ] `PcmArchive` — rolling PCM store for turn slicing
- [ ] Session wiring: closed-turn → resolve → global registry
