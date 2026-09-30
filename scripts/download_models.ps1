# Downloads Nemotron-3-Diarization GGUF weights into models/.
# Requires: pip install -U huggingface_hub, and HF_TOKEN set (env or .env).
$ErrorActionPreference = 'Stop'

$envFile = '.env'
if ((Test-Path $envFile) -and -not $env:HF_TOKEN) {
    Get-Content $envFile | ForEach-Object {
        if ($_ -match '^\s*HF_TOKEN\s*=\s*(.+)$') { $env:HF_TOKEN = $Matches[1].Trim() }
    }
}
if (-not $env:HF_TOKEN) {
    Write-Error "HF_TOKEN not set. Accept the OpenMDW license on huggingface.co/nvidia/Nemotron-3-Diarization, then set HF_TOKEN in .env"
}

New-Item -ItemType Directory -Force -Path models | Out-Null

# BF16 (best accuracy, ~190 MB). Swap for -q8_0.gguf for CPU-only machines.
hf download audio-cpp/Nemotron-3-Diarization-GGUF `
    nemotron-3-diarization-bf16.gguf `
    --local-dir models

Write-Host "Model downloaded to models\nemotron-3-diarization-bf16.gguf" -ForegroundColor Green
