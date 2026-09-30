# Packages a release zip: exe + assets + model downloader + README.
$ErrorActionPreference = 'Stop'
$out = 'release\speakerscope-win-x64'
Remove-Item -Recurse -Force $out -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null

Copy-Item build\speakerscope.exe $out\
Copy-Item -Recurse assets $out\ -ErrorAction SilentlyContinue
Copy-Item scripts\download_models.ps1 $out\
Copy-Item .env.example $out\
Copy-Item README.md, LICENSE $out\

Compress-Archive -Path $out -DestinationPath release\speakerscope-win-x64.zip -Force
Write-Host "Wrote release\speakerscope-win-x64.zip" -ForegroundColor Green
