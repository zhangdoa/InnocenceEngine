# TestPathTracerThreeScenes.ps1 - Three-scene GPU path tracer capture harness
#
# Drives the GPU path tracer headlessly across all three mandatory scenes
# (UnitTest, GITestBox, GISponza) so the visual-validation discipline's
# three-scene gate can be evidenced from a single invocation.
#
# Each scene is launched in its own Main.exe run with:
#   -test gpu_path_tracer        renderer flips into GPU PT mode
#   -scene <relative-path>       initial-scene override (TASK-210)
#   -total_frames <N>            auto-terminate after N frames
#   -dump_frames <START>-<END>   gpu_output_NNNN.png per frame in range
#   -camera_orbit <P,R,D>        optional yaw orbit for multi-angle coverage
#
# Captures land under <BinDir>/gpu_output_*.png; the script moves them into
# Build/captures/<RunTag>/{unittest,gitestbox,gisponza}/ between scenes so
# successive runs do not stomp each other's output.
#
# IMPORTANT — these knobs must stay in sync with the engine:
#   1. `-loglevel 0` keeps Success-level lines visible. The success markers
#      grep'd below ("Initial scene override:", "Auto-test: ... terminating")
#      are emitted at LogLevel::Success (Source/Engine/Common/LogService.h).
#      Loglevel >= 2 hides them and the script reports a false FAIL.
#   2. `-scene` and `-total_frames` are parsed in Source/Engine/Engine.cpp.
#      `-scene` is whitespace-terminated; do not quote the path here.
#   3. `World.inl` suppresses the frame-5 GISponza auto-switch when
#      `-scene` is set, so the override controls the whole run.
# If either side changes, update both. See TASK-210.

[CmdletBinding()]
param(
    [int]$Frames = 60,
    [int]$DumpStart = -1,
    [int]$DumpEnd = -1,
    [string]$CameraOrbit = "",
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo",
    [string]$RunTag = ""
)

$ErrorActionPreference = "Stop"

# Default the dump window to the back half of the run so accumulation has
# settled before the PNG sequence begins. Overridable via -DumpStart/-DumpEnd.
if ($DumpStart -lt 0) { $DumpStart = [int]($Frames / 2) }
if ($DumpEnd   -lt 0) { $DumpEnd   = $Frames - 1 }
if ($DumpEnd -lt $DumpStart) {
    Write-Host "FAIL - DumpEnd ($DumpEnd) < DumpStart ($DumpStart)."
    exit 1
}

if (-not $RunTag) {
    $RunTag = "three_scene_" + (Get-Date -Format "yyyyMMdd_HHmmss")
}

$mainExe = Join-Path $BinDir "Main.exe"
if (-not (Test-Path $mainExe)) {
    Write-Host "FAIL - Main.exe not found at $mainExe."
    exit 1
}

# Bin/ is the working directory for Main.exe (matches StartEngineWin.ps1)
# AND the directory the engine writes gpu_output_*.png into — IOService's
# data directory resolves relative to CWD, not to the exe location.
$binRoot = Split-Path $BinDir -Parent
$repoRoot = Split-Path $binRoot -Parent
$capturesRoot = Join-Path $repoRoot "Build\captures\$RunTag"
New-Item -ItemType Directory -Path $capturesRoot -Force | Out-Null

Set-Location $binRoot

# Per-scene table — name (output dir + log marker), scene path, success grep.
# Scene paths are relative to Bin/Data/, matching SceneService::Load
# convention. The success-grep verifies the engine actually loaded the
# requested scene rather than silently falling back to the default.
$scenes = @(
    @{
        Name     = "unittest"
        Scene    = "ExampleProject/Scenes/UnitTest.InnoScene"
        SceneTag = "UnitTest.InnoScene"
    },
    @{
        Name     = "gitestbox"
        Scene    = "ExampleProject/Scenes/GITestBox.InnoScene"
        SceneTag = "GITestBox.InnoScene"
    },
    @{
        Name     = "gisponza"
        Scene    = "ExampleProject/Scenes/GISponza.InnoScene"
        SceneTag = "GISponza.InnoScene"
    }
)

$overallPass = $true

foreach ($s in $scenes) {
    Write-Host ""
    Write-Host "=========================================="
    Write-Host "Scene: $($s.Name)  ($($s.Scene))"
    Write-Host "=========================================="

    $sceneOutDir = Join-Path $capturesRoot $s.Name
    New-Item -ItemType Directory -Path $sceneOutDir -Force | Out-Null

    $argList = "-renderer 0 -loglevel 0 -offscreen " +
               "-test gpu_path_tracer " +
               "-scene $($s.Scene) " +
               "-total_frames $Frames " +
               "-dump_frames $DumpStart-$DumpEnd"
    if ($CameraOrbit) {
        $argList += " -camera_orbit $CameraOrbit"
    }

    Write-Host "Running: $mainExe $argList"
    $proc = Start-Process `
        -FilePath $mainExe `
        -ArgumentList $argList `
        -Wait -PassThru -NoNewWindow

    Write-Host "Exit code: $($proc.ExitCode)"

    $logFile = Get-ChildItem "C:\GitRepo\InnocenceEngine\Bin\*.Log" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $logFile) {
        Write-Host "FAIL [$($s.Name)] - No log file found."
        $overallPass = $false
        continue
    }

    Write-Host "Log: $($logFile.Name)"

    $d3dErrors = Select-String -LiteralPath $logFile.FullName `
        -Pattern "D3D12 ERROR|CORRUPTION|Validation Error" -SimpleMatch

    $sceneLoaded = Select-String -LiteralPath $logFile.FullName `
        -Pattern "$($s.SceneTag) has been loaded" -SimpleMatch

    $autoTerminated = Select-String -LiteralPath $logFile.FullName `
        -Pattern "Auto-test:.*terminating"

    Write-Host "$($s.SceneTag) loaded:  $($null -ne $sceneLoaded)"
    Write-Host "Auto-terminated:           $($null -ne $autoTerminated)"
    Write-Host "D3D12 errors:              $($d3dErrors.Count)"

    $scenePass = $true
    if ($d3dErrors) {
        Write-Host "FAIL [$($s.Name)] - D3D12 errors detected:"
        $d3dErrors | ForEach-Object { Write-Host "  $_" }
        $scenePass = $false
    }
    if (-not $sceneLoaded) {
        Write-Host "FAIL [$($s.Name)] - $($s.SceneTag) was not loaded."
        $scenePass = $false
    }
    if (-not $autoTerminated) {
        Write-Host "FAIL [$($s.Name)] - engine did not auto-terminate."
        $scenePass = $false
    }

    # Move dumped PNGs into the per-scene capture directory regardless of
    # pass/fail — failure-case captures are useful diagnostic artifacts.
    # Engine writes to CWD = Bin/, not BinDir = Bin/RelWithDebInfo.
    $pngs = Get-ChildItem -Path $binRoot -Filter "gpu_output_*.png" -ErrorAction SilentlyContinue
    if ($pngs) {
        foreach ($png in $pngs) {
            Move-Item -LiteralPath $png.FullName -Destination $sceneOutDir -Force
        }
        Write-Host "Moved $($pngs.Count) capture(s) to $sceneOutDir"
    } else {
        Write-Host "WARN [$($s.Name)] - no gpu_output_*.png produced (dump range $DumpStart-$DumpEnd)."
    }

    # Stash the log alongside the captures so the per-scene evidence is
    # self-contained.
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
    Write-Host "OVERALL: PASS"
    exit 0
} else {
    Write-Host "OVERALL: FAIL"
    exit 1
}
