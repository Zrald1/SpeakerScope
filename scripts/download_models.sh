#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${HF_TOKEN:-}" && -f .env ]]; then
    export $(grep -E '^HF_TOKEN=' .env | xargs)
fi
if [[ -z "${HF_TOKEN:-}" ]]; then
    echo "HF_TOKEN not set" >&2; exit 1
fi

mkdir -p models
hf download audio-cpp/Nemotron-3-Diarization-GGUF \
    nemotron-3-diarization-bf16.gguf \
    --local-dir models
