# Build test thuat toan + adapter mms tren Windows.
# Dung g++ neu co, neu khong dung zig (pip install ziglang).
#   powershell -ExecutionPolicy Bypass -File scripts\build_host.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force build | Out-Null

if (Get-Command g++ -ErrorAction SilentlyContinue) {
    $cxx = @("g++")
} else {
    $cxx = @("python", "-m", "ziglang", "c++")
}
$core = Get-ChildItem lib/MazeCore/src/*.cpp | ForEach-Object { $_.FullName }
$flags = @("-std=c++17", "-O2", "-Wall", "-Wextra", "-Ilib/MazeCore/src")

$exe, $rest = $cxx
foreach ($t in @(
        @("test/host/test_core.cpp", "build/test_core.exe"),
        @("test/host/test_speedrun.cpp", "build/test_speedrun.exe"),
        @("sim/main_sim.cpp", "build/mouse_sim.exe"))) {
    & $exe @rest @flags @core $t[0] -o $t[1]
    if ($LASTEXITCODE -ne 0) { exit 1 }
}
Write-Host "Build xong: build\test_core.exe, build\test_speedrun.exe, build\mouse_sim.exe"

& .\build\test_core.exe
if ($LASTEXITCODE -ne 0) { exit 1 }
# Tham so: ban kinh than robot (mm) dung de tinh khe ho tuong khi chay slalom/cheo
& .\build\test_speedrun.exe 45
exit $LASTEXITCODE
