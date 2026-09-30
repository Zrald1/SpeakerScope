# lablab.ai submission draft

**Title:** SpeakerScope — real-time speaker-attributed transcription (C++ desktop)

**Short description:**
A native C++ desktop app that fuses NVIDIA Nemotron-3-Diarization (on-device,
8 speakers, overlap-aware) with AssemblyAI Universal Streaming to produce a
live color-coded transcript of *who said what* — from your mic or any app
playing audio on your PC.

**Long description:**
(expand: privacy — diarization never leaves the device; use cases — meetings,
call QA, podcast indexing, accessibility; engineering — single-sample-clock
fusion of two async inference streams, lock-free audio pipeline, custom
speaker-lane visualization.)

**Tags:** AssemblyAI, speaker-diarization, NVIDIA Nemotron, C++, real-time,
desktop, speech-to-text, streaming, meetings

**Links:** GitHub repo, GitHub Releases zip, demo video, slides.
