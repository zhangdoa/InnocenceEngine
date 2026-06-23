# TestGIScene.ps1 - Autonomous GI scene load test
#
# All launch knobs live in Data/Engine/Configuration/Presets/GIScene.json.
# This script invokes the engine and asserts: exit code 0 (primary),
# log clean / scene loaded / auto-terminated (advisory; offscreen logs are
# empty), and GPU-vs-CPU MAE within threshold when ImageMagick is healthy.

param(
    [int]$Frames = 60,
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo"
)

Import-Module (Join-Path $PSScriptRoot 'Lib\Test-Engine.psm1') -Force

$run = Invoke-EngineMainRun -BinDir $BinDir `
    -ArgList '-c Engine/Configuration/Presets/GIScene.json' `
    -NoNewWindow

$exitCode = $run.Process.ExitCode
Write-Host "Engine exit code: $exitCode"
if ($exitCode -ne 0) {
    Write-Host "FAIL - engine exited $exitCode (crash / hang / non-graceful)."
    exit 1
}

# Log-grep predicates are a hard FAIL only on a populated log (windowed runs).
# Offscreen runs leave stdout AND *.Log empty, so the exit code above is the
# primary signal and Test-EngineRunOutcome is advisory - see Scripts/CLAUDE.md.
if ($run.LogFile) {
    $outcome = Test-EngineRunOutcome -LogFile $run.LogFile -SceneTag 'GISponza.InnoScene'
    if (-not $outcome.Pass) { exit 1 }
} else {
    Write-Host "WARN - no *.Log found; relying on exit code."
}

# --- PNG comparison (best-effort; needs a HEALTHY ImageMagick) ---
$gpuPng = Join-Path $run.BinRoot "gpu_output.png"
$cpuPng = Join-Path $run.BinRoot "cpu_reference.png"

# Probe magick HEALTH, not just presence: a present-but-broken magick
# (magick -version exit != 0) otherwise hangs / fails the compare below
# (observed 2026-06-23: magick -version exited 5 and every op stalled).
$magickOk = $false
if (Get-Command 'magick' -ErrorAction SilentlyContinue) {
    & magick -version *> $null
    $magickOk = ($LASTEXITCODE -eq 0)
}
if (-not $magickOk) {
    Write-Host "WARN - ImageMagick missing or broken; skipping MAE comparison."
    Write-Host "PASS (exit-code + log assertions only)."
    exit 0
}

foreach ($f in @($gpuPng, $cpuPng)) {
    if (-not (Test-Path $f)) {
        Write-Host "FAIL - missing required file: $f"
        exit 1
    }
}

$identifyText = (magick identify -verbose $gpuPng 2>&1) | Out-String
$maxVal = [regex]::Match($identifyText, 'max:\s+[\d.]+\s+\(([\d.]+)\)')
if (-not $maxVal.Success -or $maxVal.Groups[1].Value -match "infinity|undefined") {
    Write-Host "WARN - Could not confirm GPU output max channel value."
}

$gpuResized = Join-Path $run.BinRoot "gpu_output_resized.png"
$cpuDims = (magick identify -format "%wx%h" $cpuPng 2>&1) | Out-String
$cpuDims = $cpuDims.Trim()
magick $gpuPng -resize $cpuDims $gpuResized

$maeLine = (magick compare -metric MAE $gpuResized $cpuPng null: 2>&1) | Out-String
$maeMatch = [regex]::Match($maeLine, '\(([\d.]+)\)')
if (-not $maeMatch.Success) {
    Write-Host "FAIL - could not parse MAE value from: $maeLine"
    exit 1
}
$mae = [float]$maeMatch.Groups[1].Value

$maeThreshold = 0.45
Write-Host "MAE:              $mae  (threshold: $maeThreshold)"

if ($mae -gt $maeThreshold) {
    Write-Host "FAIL - MAE exceeds threshold."
    exit 1
}

Write-Host 'PASS'
exit 0
