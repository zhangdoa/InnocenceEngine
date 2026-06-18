# TestPT.ps1 - Autonomous GPU path tracer smoke test
#
# All launch knobs (offscreen, loglevel, totalFrames, testCase, initialScene)
# live in Data/Engine/Configuration/Presets/PT.json. This script just invokes
# the engine with that config and asserts the standard pass conditions from
# the engine log.

param(
    [int]$Frames = 60,
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo"
)

Import-Module (Join-Path $PSScriptRoot 'Lib\Test-Engine.psm1') -Force

$run = Invoke-EngineMainRun -BinDir $BinDir `
    -ArgList '-c Engine/Configuration/Presets/PT.json' `
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
