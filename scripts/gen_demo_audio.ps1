# Generates a multi-"speaker" demo wav via Windows SAPI voices + gold labels.
# Output: assets/samples/meeting.wav (16 kHz s16 mono) + assets/samples/meeting.gold.json
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Speech

$script:lines = @(
    @{ voice = 'Microsoft Zira Desktop';  text = 'Good morning everyone, thanks for joining the product sync. Today we are reviewing the speaker timeline feature.' },
    @{ voice = 'Microsoft George';        text = 'Thanks Zira. Quick update from my side: the diarization model now supports up to eight simultaneous speakers.' },
    @{ voice = 'Microsoft Hazel';         text = 'That is great news. For the hackathon demo, can we also show overlapping speech detection?' },
    @{ voice = 'Microsoft Zira Desktop';  text = 'Yes, Nemotron outputs per frame probabilities, so overlapping talkers both show up in the timeline.' },
    @{ voice = 'Microsoft Susan';         text = 'One question from the design team: can users rename the detected speakers in the interface?' },
    @{ voice = 'Microsoft George';        text = 'Absolutely. Each lane has an editable name field, and the transcript can be filtered per person.' },
    @{ voice = 'Microsoft Hazel';         text = 'Perfect. I think that covers everything for the demo. Let us wrap up and ship it.' }
)

$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(
    16000, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,
    [System.Speech.AudioFormat.AudioChannel]::Mono)

New-Item -ItemType Directory -Force -Path 'assets\samples' | Out-Null
$tmpDir = 'assets\samples\tmp_sapi'
New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null

$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
$synth.Rate = 0
$pcm = [System.Collections.Generic.List[byte]]::new()
$gold = [System.Collections.Generic.List[object]]::new()
$cursorMs = 0
$gapMs = 450

$voiceIdx = @{}
$nextSpk = 0
$i = 0
foreach ($line in $script:lines) {
    if (-not $voiceIdx.ContainsKey($line.voice)) {
        $voiceIdx[$line.voice] = $nextSpk; $nextSpk++
    }
    $spk = $voiceIdx[$line.voice]
    $out = Join-Path $tmpDir "u$i.wav"
    $synth.SelectVoice($line.voice)
    $synth.SetOutputToWaveFile($out, $fmt)
    $synth.Speak($line.text)
    $synth.SetOutputToNull()

    $bytes = [System.IO.File]::ReadAllBytes($out)
    # find 'data' chunk
    $off = 12
    $dataOff = -1; $dataLen = 0
    while ($off -lt $bytes.Length - 8) {
        $tag = [System.Text.Encoding]::ASCII.GetString($bytes, $off, 4)
        $len = [BitConverter]::ToInt32($bytes, $off + 4)
        if ($tag -eq 'data') { $dataOff = $off + 8; $dataLen = $len; break }
        $off += 8 + $len
    }
    if ($dataOff -lt 0) { throw "no data chunk in $out" }
    $durMs = [int](($dataLen / 2) * 1000 / 16000)

    for ($j = 0; $j -lt $dataLen; $j++) { $pcm.Add($bytes[$dataOff + $j]) }
    $gold.Add(@{ speaker = $spk; start_ms = $cursorMs; end_ms = ($cursorMs + $durMs) })
    $cursorMs += $durMs
    # silence gap
    for ($j = 0; $j -lt ($gapMs * 32); $j++) { $pcm.Add(0) }
    $cursorMs += $gapMs
    $i++
}
$synth.Dispose()

# Write WAV header (16kHz s16 mono) + payload
$outWav = 'assets\samples\meeting.wav'
$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter($ms)
$dataSize = $pcm.Count
$bw.Write([System.Text.Encoding]::ASCII.GetBytes('RIFF'))
$bw.Write([int](36 + $dataSize))
$bw.Write([System.Text.Encoding]::ASCII.GetBytes('WAVE'))
$bw.Write([System.Text.Encoding]::ASCII.GetBytes('fmt '))
$bw.Write([int]16); $bw.Write([int16]1); $bw.Write([int16]1)
$bw.Write([int]16000); $bw.Write([int]32000); $bw.Write([int16]2); $bw.Write([int16]16)
$bw.Write([System.Text.Encoding]::ASCII.GetBytes('data'))
$bw.Write([int]$dataSize)
$bw.Write($pcm.ToArray())
$bw.Close()
[System.IO.File]::WriteAllBytes($outWav, $ms.ToArray())

$gold | ConvertTo-Json | Set-Content 'assets\samples\meeting.gold.json'
Remove-Item -Recurse -Force $tmpDir
Write-Host "Wrote $outWav ($([math]::Round($dataSize/32000,1))s, $($voiceIdx.Count) voices) + gold.json" -ForegroundColor Green
