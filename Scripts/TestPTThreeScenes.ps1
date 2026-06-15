# TestPTThreeScenes.ps1 - Three-scene GPU path tracer capture harness
#
# All base launch knobs (offscreen, loglevel, testCase, camera orbit, dump
# frame pattern) live in Data/Engine/Configuration/Presets/PTThreeScene.json.
# Per-scene initialScene is a config override written to a temp file; the
# scene override loop is data-driven end to end.

[CmdletBinding()]
param(
    [int]$Frames = 60,
    [int]$DumpStart = -1,
    [int]$DumpEnd = -1,
    [int]$ExtraHeadroom = 20,
    [int]$FlapBackSafetyMargin = 5,
    [string]$RunTag = "",
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo"
)

$ErrorActionPreference = "Stop"

Import-Module (Join-Path $PSScriptRoot 'Lib\Test-Engine.psm1') -Force

if ($DumpStart -lt 0) { $DumpStart = [int]($Frames / 2) }
if ($DumpEnd   -lt 0) { $DumpEnd   = $Frames - 1 }
if ($DumpEnd -lt $DumpStart) { throw "-DumpEnd ($DumpEnd) < -DumpStart ($DumpStart)" }
if ($ExtraHeadroom -lt 0) { $ExtraHeadroom = 20 }
if ($FlapBackSafetyMargin -lt 0) { $FlapBackSafetyMargin = 5 }

$engineDumpEnd = [Math]::Min($DumpEnd + $ExtraHeadroom, $Frames - 1)
$dumpLen = $DumpEnd - $DumpStart + 1

if (-not $RunTag) {
    $RunTag = "three_scene_" + (Get-Date -Format "yyyyMMdd_HHmmss")
}

$mainExe = Join-Path $BinDir "Main.exe"
if (-not (Test-Path $mainExe)) { throw "Main.exe not found at $mainExe" }

$binRoot = Split-Path $BinDir -Parent
$repoRoot = Split-Path $binRoot -Parent
$capturesRoot = Join-Path $repoRoot "Build\captures\$RunTag"
New-Item -ItemType Directory -Path $capturesRoot -Force | Out-Null

Set-Location $binRoot

$basePreset = Join-Path $repoRoot "Data/Engine/Configuration/Presets/PTThreeScene.json"
$presetsOut = Join-Path $binRoot "_presets"
New-Item -ItemType Directory -Path $presetsOut -Force | Out-Null

$scenes = @(
    @{ Name = "unittest";   Scene = "ExampleProject/Scenes/UnitTest.InnoScene"   },
    @{ Name = "gitestbox";  Scene = "ExampleProject/Scenes/GITestBox.InnoScene"  },
    @{ Name = "gisponza";   Scene = "ExampleProject/Scenes/GISponza.InnoScene"   }
)

function Write-ScenePreset {
    param([string]$Base, [string]$Scene, [int]$DS, [int]$DE, [string]$Out)
    $j = Get-Content -Raw -Path $Base | ConvertFrom-Json
    $j.session.totalFrames = $Frames
    if (-not $j.session) { $j | Add-Member -NotePropertyName session -NotePropertyValue (New-Object PSObject) }
    $j.session | Add-Member -NotePropertyName dumpFramesStart -NotePropertyValue $DS -Force
    $j.session | Add-Member -NotePropertyName dumpFramesEnd   -NotePropertyValue $DE -Force
    if (-not $j.scene) { $j | Add-Member -NotePropertyName scene -NotePropertyValue (New-Object PSObject) }
    $j.scene.initialScene = $Scene
    $j | ConvertTo-Json -Depth 16 | Set-Content -Path $Out -Encoding UTF8
}

$overallPass = $true

foreach ($s in $scenes) {
    Write-Host ""
    Write-Host "=========================================="
    Write-Host "Scene: $($s.Name)  ($($s.Scene))"
    Write-Host "=========================================="

    $sceneOutDir = Join-Path $capturesRoot $s.Name
    New-Item -ItemType Directory -Path $sceneOutDir -Force | Out-Null

    $scenePreset = Join-Path $presetsOut "PTThreeScene_$($s.Name).json"
    Write-ScenePreset -Base $basePreset -Scene $s.Scene -DS $DumpStart -DE $engineDumpEnd -Out $scenePreset

    $argList = "-c $scenePreset"
    $run = Invoke-EngineMainRun -BinDir $BinDir -ArgList $argList -NoNewWindow

    if (-not $run.LogFile) {
        Write-Host "FAIL [$($s.Name)] - No log file found."
        $overallPass = $false
        continue
    }

    $logFile = $run.LogFile
    $outcome = Test-EngineRunOutcome -LogFile $logFile -SceneTag ($s.Scene -replace '^.*/', '') -ScenePrefix $s.Name

    $steadyStateReached = Select-String -LiteralPath $logFile.FullName -Pattern "Auto-test: steady state reached at frame=(\d+)"
    $steadyStateTimeout = Select-String -LiteralPath $logFile.FullName -Pattern "Auto-test: steady state NOT reached within"
    $steadyStateLost = Select-String -LiteralPath $logFile.FullName -Pattern "Auto-test: steady state lost at frame=(\d+)"

    Write-Host "Steady-state reached:      $($null -ne $steadyStateReached)"
    Write-Host "Steady-state timeout:      $($null -ne $steadyStateTimeout)"
    Write-Host "Steady-state lost events:  $(@($steadyStateLost).Count)"

    $scenePass = $outcome.Pass
    if (-not $steadyStateReached) {
        Write-Host "FAIL [$($s.Name)] - missing steady-state-reached log marker."
        $scenePass = $false
    }
    if ($steadyStateTimeout) {
        Write-Host "FAIL [$($s.Name)] - 120-frame steady-state timeout warning fired."
        $scenePass = $false
    }

    $effectiveStart = $DumpStart
    $effectiveEnd = $DumpEnd
    $shiftReason = ""
    if ($steadyStateReached) {
        $steadyFrame = [int]$steadyStateReached.Matches[0].Groups[1].Value
        if ($steadyStateLost) {
            $lostFrames = @($steadyStateLost) | ForEach-Object { [int]$_.Matches[0].Groups[1].Value }
            $lastFlapFrame = ($lostFrames | Measure-Object -Maximum).Maximum
            $threshold = ($lastFlapFrame - $steadyFrame) + $FlapBackSafetyMargin
            if ($threshold -gt $effectiveStart) {
                $effectiveStart = $threshold
                $effectiveEnd = $threshold + ($dumpLen - 1)
                $shiftReason = "flap-back at FCSL=$lastFlapFrame shifted window to [$effectiveStart, $effectiveEnd]"
            }
        }
    }
    $captureCeiling = $engineDumpEnd
    if ($effectiveEnd -gt $captureCeiling) {
        Write-Host "FAIL [$($s.Name)] - flap-back pushed end ($effectiveEnd) past ceiling ($captureCeiling). Rerun with longer -Frames / -ExtraHeadroom. Reason: $shiftReason"
        $scenePass = $false
    } elseif ($shiftReason) {
        Write-Host "INFO [$($s.Name)] - $shiftReason"
    } else {
        Write-Host "INFO [$($s.Name)] - no flap-back shift (effective window [$effectiveStart, $effectiveEnd])"
    }

    $pngs = Get-ChildItem -Path $binRoot -Filter "gpu_output_*.png" -ErrorAction SilentlyContinue
    if ($pngs) {
        foreach ($png in $pngs) {
            Move-Item -LiteralPath $png.FullName -Destination $sceneOutDir -Force
        }
        Write-Host "Moved $($pngs.Count) capture(s) to $sceneOutDir"
    } else {
        Write-Host "WARN [$($s.Name)] - no gpu_output_*.png produced."
    }

    $trimmedDir = Join-Path $sceneOutDir "_trimmed"
    $trimmedCount = 0
    foreach ($png in Get-ChildItem -Path $sceneOutDir -Filter "gpu_output_*.png" -ErrorAction SilentlyContinue) {
        if ($png.BaseName -match 'gpu_output_(\d+)') {
            $frameIdx = [int]$Matches[1]
            if ($frameIdx -lt $effectiveStart -or $frameIdx -gt $effectiveEnd) {
                if (-not (Test-Path $trimmedDir)) {
                    New-Item -ItemType Directory -Path $trimmedDir -Force | Out-Null
                }
                Move-Item -LiteralPath $png.FullName -Destination $trimmedDir -Force
                $trimmedCount++
            }
        }
    }
    if ($trimmedCount -gt 0) {
        Write-Host "Trimmed $trimmedCount frame(s) outside [$effectiveStart, $effectiveEnd] to $trimmedDir"
    }

    Copy-Item -LiteralPath $logFile.FullName -Destination (Join-Path $sceneOutDir "engine.log") -Force

    if (-not $scenePass) {
        $overallPass = $false
    } else {
        Write-Host "PASS [$($s.Name)]"
    }
}

Write-Host ""
Write-Host "=========================================="
Write-Host "Three-scene capture run: $RunTag"
Write-Host "Captures under: $capturesRoot"
if ($overallPass) {
    Write-Host "RESULT: PASS"
    exit 0
} else {
    Write-Host "RESULT: FAIL"
    exit 1
}
