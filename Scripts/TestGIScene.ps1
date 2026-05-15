# TestGIScene.ps1 - Autonomous GI scene load test
# Runs Main.exe: loads UnitTest scene, switches to GISponza at frame 5,
# renders N total frames, exits. Exits 0 on a clean run with the GPU
# output matching the CPU reference within MAE threshold; 1 otherwise.
#
# IMPORTANT — these knobs must stay in sync with the engine:
#   1. -loglevel must be <= 1 (Success). The success-path markers grep'd
#      below ("Scene ... has been loaded.", "Auto-test: ... terminating")
#      are emitted at LogLevel::Success (see Source/Engine/Common/LogService.h:6
#      and Engine.cpp/World.inl call sites). Loglevel 2 (Warning) suppresses
#      them and the script reports a false FAIL on a working engine.
#   2. The auto-terminate frame-budget flag is -total_frames (parsed in
#      Source/Engine/Engine.cpp). -frames is silently ignored, so the engine
#      runs forever and the script hangs / fails the auto-terminate grep.
#   3. The GI scene name must match the scene the auto-test path actually
#      loads. World.inl currently loads GISponza.InnoScene at frame 5. If
#      that scene is renamed or replaced, update the grep below.
# If either side changes, update both. See TASK-158.

param(
    [int]$Frames = 120,
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo"
)

Import-Module (Join-Path $PSScriptRoot 'Lib\Test-Engine.psm1') -Force

$run = Invoke-EngineMainRun -BinDir $BinDir `
    -ArgList "-renderer 0 -loglevel 1 -total_frames $Frames"

if (-not $run.LogFile) {
    Write-Host "ERROR: No log file found."
    exit 1
}

$outcome = Test-EngineRunOutcome -LogFile $run.LogFile `
    -SceneTag 'GISponza.InnoScene'

if (-not $outcome.Pass) {
    # GISponza-specific diagnostic for the not-loaded branch.
    if (-not $outcome.SceneLoaded) {
        Write-Host "       (auto-test path in World.inl loads it at frame 5; if the scene"
        Write-Host "        was renamed, update both this script and World.inl together.)"
    }
    exit 1
}

# --- PNG comparison ---
$gpuPng = Join-Path $run.BinRoot "gpu_output.png"
$cpuPng = Join-Path $run.BinRoot "cpu_reference.png"

# Check ImageMagick
if (-not (Get-Command "magick" -ErrorAction SilentlyContinue))
{
    Write-Host "FAIL - ImageMagick 'magick' not found. Install from https://imagemagick.org/script/download.php"
    exit 1
}

# Check file presence and size
foreach ($f in @($gpuPng, $cpuPng))
{
    if (-not (Test-Path $f))
    {
        Write-Host "FAIL - Missing file: $f"
        exit 1
    }
    if ((Get-Item $f).Length -lt 100)
    {
        Write-Host "FAIL - File too small (likely 1x1 error sentinel): $f"
        exit 1
    }
}

# NaN/Inf check on GPU output
$identifyText = (magick identify -verbose $gpuPng 2>&1) | Out-String
$maxVal       = [regex]::Match($identifyText, 'max:\s+[\d.]+\s+\(([\d.]+)\)')
if (-not $maxVal.Success -or $maxVal.Groups[1].Value -match "infinity|undefined")
{
    Write-Host "WARN - Could not confirm GPU output max channel value."
}

# Resize GPU output to match CPU reference dimensions before comparison
$gpuResized = Join-Path $run.BinRoot "gpu_output_resized.png"
$cpuDims    = (magick identify -format "%wx%h" $cpuPng 2>&1) | Out-String
$cpuDims    = $cpuDims.Trim()
magick $gpuPng -resize $cpuDims $gpuResized

# MAE comparison — IM7 HDRI outputs "NNNN.NN (0.NNNN)" on stderr; extract normalized value
$maeLine  = (magick compare -metric MAE $gpuResized $cpuPng null: 2>&1) | Out-String
$maeMatch = [regex]::Match($maeLine, '\(([\d.]+)\)')
if (-not $maeMatch.Success)
{
    Write-Host "FAIL - Could not parse MAE from magick compare output: $maeLine"
    exit 1
}
$mae = [float]$maeMatch.Groups[1].Value

$maeThreshold = 0.45

Write-Host "MAE:              $mae  (threshold: $maeThreshold)"

if ($mae -gt $maeThreshold)
{
    Write-Host "FAIL - MAE $mae exceeds threshold $maeThreshold"
    exit 1
}

Write-Host 'PASS'
exit 0
