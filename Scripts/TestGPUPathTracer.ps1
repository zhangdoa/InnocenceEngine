# TestGPUPathTracer.ps1 - Autonomous GPU path tracer smoke test
# Runs Main.exe in offscreen mode with -test gpu_path_tracer: the auto-test
# path loads UnitTest, switches to GISponza at frame 5 (World.inl), and the
# rendering client flips into GPU path tracer mode at startup
# (ExampleRenderingClient.cpp). Renders N total frames, exits.
# Exits 0 if no D3D12 errors found and the success-path markers fire,
# 1 otherwise.
#
# IMPORTANT — these knobs must stay in sync with the engine:
#   1. -loglevel must be <= 1 (Success). The success-path markers grep'd
#      below ("GISponza.InnoScene has been loaded.", "Auto-test: ...
#      terminating") are emitted at LogLevel::Success (see
#      Source/Engine/Common/LogService.h:6 and Engine.cpp/World.inl call
#      sites). Loglevel 2 (Warning) suppresses them and the script reports
#      a false FAIL on a working engine.
#   2. The auto-terminate frame-budget flag is -total_frames (parsed in
#      Source/Engine/Engine.cpp). -frames is silently ignored, so the engine
#      runs forever and the script hangs / fails the auto-terminate grep.
#   3. The scene name must match the scene the auto-test path actually
#      loads. World.inl currently loads GISponza.InnoScene at frame 5 for
#      both the default and -test gpu_path_tracer flows. If that scene is
#      renamed or replaced, update the grep below.
# If either side changes, update both. See TASK-159.

param(
    [int]$Frames = 60,
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo"
)

Import-Module (Join-Path $PSScriptRoot 'Lib\Test-Engine.psm1') -Force

$run = Invoke-EngineMainRun -BinDir $BinDir `
    -ArgList "-renderer 0 -loglevel 0 -offscreen -total_frames $Frames -test gpu_path_tracer" `
    -NoNewWindow

if (-not $run.LogFile) {
    Write-Host "FAIL - No log file found."
    exit 1
}

$outcome = Test-EngineRunOutcome -LogFile $run.LogFile `
    -SceneTag 'GISponza.InnoScene'

if (-not $outcome.Pass) {
    if (-not $outcome.SceneLoaded) {
        Write-Host "       (auto-test path in World.inl loads it at frame 5; if the scene"
        Write-Host "        was renamed, update both this script and World.inl together.)"
    }
    exit 1
}

Write-Host "PASS"
exit 0
