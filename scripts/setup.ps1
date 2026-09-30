# SpeakerScope build setup (Windows)
$ErrorActionPreference = 'Stop'

$VcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { 'C:\vcpkg' }
$Toolchain = Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake'

if (!(Test-Path $Toolchain)) {
    Write-Error "vcpkg toolchain not found at $Toolchain. Set VCPKG_ROOT or install vcpkg to C:\vcpkg."
}

cmake -B build -G Ninja `
    -DCMAKE_TOOLCHAIN_FILE="$Toolchain" `
    -DVCPKG_TARGET_TRIPLET=x64-windows `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build build
Write-Host "`nDone. Run: .\build\speakerscope.exe" -ForegroundColor Green
