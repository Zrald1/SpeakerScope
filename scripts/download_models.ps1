# Downloads Nemotron-3-Diarization GGUF weights into models/.
# The audio-cpp mirror is public — no HF token required.
$ErrorActionPreference = 'Stop'

$Repo = 'audio-cpp/Nemotron-3-Diarization-GGUF'
# q8_0 (~102 MB): fast enough for realtime CPU inference.
# bf16 (~190 MB) is also public — swap the filename for best accuracy.
$File = 'nemotron-3-diarization-q8_0.gguf'
$Url  = "https://huggingface.co/$Repo/resolve/main/$File"

New-Item -ItemType Directory -Force -Path models | Out-Null
$out = "models\$File"
if (Test-Path $out) {
    Write-Host "$File already present" -ForegroundColor Yellow
} else {
    Write-Host "Downloading $File (~102 MB)..."
    Invoke-WebRequest -Uri $Url -OutFile $out
}
Write-Host "Model ready: $out" -ForegroundColor Green
