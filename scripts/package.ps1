# Packages a release zip: exe + runtime DLLs + model + assets + README.
$ErrorActionPreference = 'Stop'
$out = 'release\speakerscope-win-x64'
Remove-Item -Recurse -Force $out -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
New-Item -ItemType Directory -Force -Path "$out\models" | Out-Null

$build = 'build-msvc'
Copy-Item "$build\speakerscope.exe" $out\
Copy-Item "$build\audiocpp.dll", "$build\fmt.dll", "$build\glfw3.dll",
         "$build\spdlog.dll", "$build\zlib1.dll" $out\
Copy-Item "models\nemotron-3-diarization-q8_0.gguf" "$out\models\"
Copy-Item -Recurse assets $out\ -ErrorAction SilentlyContinue
Copy-Item scripts\download_models.ps1 $out\
Copy-Item .env.example $out\
Copy-Item README.md, LICENSE $out\

Compress-Archive -Path $out -DestinationPath release\speakerscope-win-x64.zip -Force
Write-Host "Wrote release\speakerscope-win-x64.zip" -ForegroundColor Green
