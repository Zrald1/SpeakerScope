# SpeakerScope

**Every voice, attributed.**

A native C++ desktop app that produces live, speaker-attributed transcripts by fusing
[NVIDIA Nemotron-3-Diarization](https://huggingface.co/nvidia/Nemotron-3-Diarization)
(on-device, up to 8 speakers, overlap-aware) with
[AssemblyAI Universal Streaming](https://www.assemblyai.com/docs/streaming/getting-started/transcribe-streaming-audio)
(cloud STT with word-level timestamps).

Built for the lablab.ai × AssemblyAI hackathon (Sept 2026).

## Pipeline

```
mic / WASAPI loopback
        │  16 kHz mono PCM (s16le)
        ├──► Nemotron-3-Diarization (on-device GGUF, C++ ggml) ──► "who spoke when"
        └──► AssemblyAI streaming v3 WebSocket ────────────────► "what was said" (word ts)
                                │
                     SpeakerAttributor (timestamp fusion)
                                │
              color-coded speaker transcript + speaker lanes
```

## Quick start (Windows)

```powershell
# prerequisites: MSVC 2022, CMake >= 3.26, vcpkg at C:\vcpkg
.\scripts\setup.ps1
.\scripts\download_models.ps1   # downloads Nemotron GGUF weights to models\
copy .env.example .env          # then add your ASSEMBLYAI_API_KEY
.\build\speakerscope.exe
```

## License

MIT (app code). Model weights are governed by OpenMDW-1.1 — see `models/` after download.
