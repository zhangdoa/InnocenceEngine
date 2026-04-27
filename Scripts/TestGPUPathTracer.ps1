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

$mainExe = Join-Path $BinDir "Main.exe"
Set-Location (Split-Path $BinDir -Parent)

Write-Host "Running: $mainExe -renderer 0 -loglevel 0 -offscreen -total_frames $Frames -test gpu_path_tracer"

$proc = Start-Process `
    -FilePath $mainExe `
    -ArgumentList "-renderer 0 -loglevel 0 -offscreen -total_frames $Frames -test gpu_path_tracer" `
    -Wait -PassThru -NoNewWindow

Write-Host "Exit code: $($proc.ExitCode)"

$logFile = Get-ChildItem "C:\GitRepo\InnocenceEngine\Bin\*.Log" |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $logFile) {
    Write-Host "FAIL - No log file found."
    exit 1
}

Write-Host "Log: $($logFile.Name)"

$d3dErrors = Select-String -LiteralPath $logFile.FullName `
    -Pattern "D3D12 ERROR|CORRUPTION|Validation Error" -SimpleMatch

$sceneLoaded = Select-String -LiteralPath $logFile.FullName `
    -Pattern "GISponza.InnoScene has been loaded" -SimpleMatch

$autoTerminated = Select-String -LiteralPath $logFile.FullName `
    -Pattern "Auto-test:.*terminating"

Write-Host "GISponza loaded:  $($null -ne $sceneLoaded)"
Write-Host "Auto-terminated:  $($null -ne $autoTerminated)"
Write-Host "D3D12 errors:     $($d3dErrors.Count)"

if ($d3dErrors) {
    Write-Host "FAIL - D3D12 errors detected:"
    $d3dErrors | ForEach-Object { Write-Host "  $_" }
    exit 1
}

if (-not $sceneLoaded) {
    Write-Host "FAIL - GISponza.InnoScene was not loaded."
    Write-Host "       (auto-test path in World.inl loads it at frame 5; if the scene"
    Write-Host "        was renamed, update both this script and World.inl together.)"
    exit 1
}

if (-not $autoTerminated) {
    Write-Host "FAIL - engine did not auto-terminate (crashed or hung?)."
    exit 1
}

Write-Host "PASS"
exit 0
