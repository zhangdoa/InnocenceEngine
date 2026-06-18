# TestGIScene.ps1 - Autonomous GI scene load test
#
# All launch knobs live in Data/Engine/Configuration/Presets/GIScene.json.
# This script invokes the engine and asserts: log clean, scene loaded,
# engine auto-terminated, GPU-vs-CPU MAE within threshold.

param(
    [int]$Frames = 60,
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo"
)

Import-Module (Join-Path $PSScriptRoot 'Lib\Test-Engine.psm1') -Force

$run = Invoke-EngineMainRun -BinDir $BinDir `
    -ArgList '-c Engine/Configuration/Presets/GIScene.json' `
    -NoNewWindow

if (-not $run.LogFile) {
    Write-Host "FAIL - No log file found."
    exit 1
}

$outcome = Test-EngineRunOutcome -LogFile $run.LogFile `
    -SceneTag 'GISponza.InnoScene'

if (-not $outcome.Pass) {
    exit 1
}

# --- PNG comparison ---
$gpuPng = Join-Path $run.BinRoot "gpu_output.png"
$cpuPng = Join-Path $run.BinRoot "cpu_reference.png"

if (-not (Get-Command "magick" -ErrorAction SilentlyContinue)) {
    Write-Host "WARN - ImageMagick not found, skipping MAE comparison."
    Write-Host "PASS (log assertions only)."
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
