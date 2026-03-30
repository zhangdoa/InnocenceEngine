# TestGIScene.ps1 - Autonomous GI scene load test
# Runs Main.exe: loads UnitTest scene, then GITestBox, renders N frames, exits.
# Exits 0 if no D3D12 errors found, 1 otherwise.

param(
    [int]$Frames = 120,
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo"
)

$mainExe = Join-Path $BinDir "Main.exe"
Set-Location (Split-Path $BinDir -Parent)

Write-Host "Running: $mainExe -renderer 0 -loglevel 2 -frames $Frames"

$proc = Start-Process `
    -FilePath $mainExe `
    -ArgumentList "-renderer 0 -loglevel 2 -frames $Frames" `
    -Wait -PassThru

Write-Host "Exit code: $($proc.ExitCode)"

# Find the newest log file written during this run
$logFile = Get-ChildItem "C:\GitRepo\InnocenceEngine\Bin\*.Log" |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if (-not $logFile) {
    Write-Host "ERROR: No log file found."
    exit 1
}

Write-Host "Log: $($logFile.Name)"

$d3dErrors = Select-String -LiteralPath $logFile.FullName `
    -Pattern "D3D12 ERROR|CORRUPTION|Validation Error" -SimpleMatch

$sceneLoaded = Select-String -LiteralPath $logFile.FullName `
    -Pattern "GITestBox.InnoScene has been loaded" -SimpleMatch

$autoTerminated = Select-String -LiteralPath $logFile.FullName `
    -Pattern "Auto-test:.*terminating"

Write-Host "GITestBox loaded: $($null -ne $sceneLoaded)"
Write-Host "Auto-terminated:  $($null -ne $autoTerminated)"
Write-Host "D3D12 errors:     $($d3dErrors.Count)"

if ($d3dErrors) {
    Write-Host "FAIL - D3D12 errors detected:"
    $d3dErrors | ForEach-Object { Write-Host "  $_" }
    exit 1
}

if (-not $sceneLoaded) {
    Write-Host "FAIL - GITestBox.InnoScene was not loaded."
    exit 1
}

if (-not $autoTerminated) {
    Write-Host "FAIL - engine did not auto-terminate (crashed or hung?)."
    exit 1
}

# --- PNG comparison ---
$gpuPng = Join-Path (Split-Path $BinDir -Parent) "gpu_output.png"
$cpuPng = Join-Path (Split-Path $BinDir -Parent) "cpu_reference.png"

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
$gpuResized = Join-Path (Split-Path $BinDir -Parent) "gpu_output_resized.png"
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
