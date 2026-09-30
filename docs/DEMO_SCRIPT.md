# Demo script (video, ~3 min)

1. **Cold open (15 s):** play a 4-person podcast clip. Narrate: "Four voices. Who said what?"
2. **Problem (20 s):** show a plain transcript — correct words, zero attribution. "Search, action items, and analytics all need *who*."
3. **SpeakerScope live (60 s):**
   - Source: System audio (loopback) → Start.
   - Speaker lanes light up channel by channel (arrival order).
   - Transcript renders color-coded per speaker; point out an overlap flag during crosstalk.
   - Rename Speaker 1 → "Host".
4. **Mic mode (20 s):** switch to microphone, two people talk briefly in-room.
5. **Tech slide (30 s):** architecture diagram — Nemotron-3-Diarization on-device (GGUF, C++ ggml) + AssemblyAI Universal Streaming v3 + timestamp fusion.
6. **Close (15 s):** export transcript.txt, show it. "Every voice, attributed."
