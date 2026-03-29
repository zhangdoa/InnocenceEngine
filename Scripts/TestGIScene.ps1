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

Write-Host 'PASS'
exit 0
