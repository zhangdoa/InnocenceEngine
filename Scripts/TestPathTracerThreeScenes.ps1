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
#
# TASK-213 CL D — engine-side log markers grep'd here. Wording is load-
# bearing and must stay in sync with FrameManagementServiceImpl.cpp:
#   "Auto-test: steady state reached at frame=N ..."  (Verbose, once per run)
#       — REQUIRED. Absence FAILs the scene; without the latch the dump
#       counter never starts (CL B), so any captures would be undefined.
#   "Auto-test: steady state lost at frame=N (instanceCount changed M->N)"
#       (Verbose, zero or more) — flap-back markers fired when the predicate
#       loses TLAS-stability after first-true. The script consumes these to
#       shift the dump window past post-latch instability (AC-7 / CL D
#       workstream 2). Parser key: "steady state lost at frame=" + integer.
#   "Auto-test: steady state NOT reached within N frames ..."  (Warning)
#       — FAIL marker. The 120-frame timeout watchdog. Capture is unreliable.
# If the engine reword these, update the corresponding patterns below.

[CmdletBinding()]
param(
    [int]$Frames = 60,
    [int]$DumpStart = -1,
    [int]$DumpEnd = -1,
    # TASK-213 CL D: extra capture-frame headroom past the user-requested
    # DumpEnd. The engine captures `[DumpStart, DumpEnd + ExtraHeadroom]`
    # and the script post-trims to a window of length (DumpEnd-DumpStart+1)
    # starting at `max(DumpStart, trustworthy_threshold)`, where the
    # threshold is computed from the engine's `steady state lost` flap-back
    # log markers (AC-7). 10 frames is enough to absorb the GISponza K=3
    # plateau drift + safety margin observed in CL D run1 (Lmax=28, S=6,
    # threshold=28-6+FlapBackSafetyMargin=27 with FlapBackSafetyMargin=5;
    # DumpStart=25 → shift by +2 → window [27,31] with margin to +10).
    # Set to 0 to disable headroom (the script will then FAIL hard whenever
    # flap-back pushes trustworthy past DumpEnd).
    [int]$ExtraHeadroom = 10,
    # TASK-213 CL D: safety margin in capture-frames added past the last
    # observed flap-back. The "Auto-test: steady state lost at frame=N" log
    # fires when IsSteadyState observes the instance-count change — one
    # frame after the actual TLAS rebuild kicks off. The rebuild itself
    # plus DX12 frame-in-flight buffering (typically 3) plus a PT accum
    # reset can take several frames to fully settle. Empirical evidence
    # from CL D run1 GISponza toggle 0 (last lost at FCSL=28, last actual
    # TLAS rebuild at FCSL=27, capture-frame 25-26 still uniform-black,
    # capture-frame 27 first to render real content) → +5 cap-frames past
    # the observed Lmax is the minimum safe margin. Tune up if a future
    # scene shows similar black-frame artifacts past the trustworthy shift.
    [int]$FlapBackSafetyMargin = 5,
    # TASK-213 CL C: default per-scene -camera_orbit pin. Empty string here
    # means "use the per-scene default below"; "none" disables the override
    # entirely (interactive / scene-file-camera path); any other value is
    # forwarded to Main.exe verbatim and overrides every scene.
    # Default triple "20,8,120" (PITCH=20deg, RADIUS=8, DURATION=120 frames)
    # matches the long-standing precedent across TASK-124/TASK-138/TASK-6.x
    # capture runs. Combined with CL B's steady-state-relative frame counter
    # in World.inl, this produces a deterministic camera viewpoint at the
    # same dump frame across binary launches (RC-6 in the TASK-213 design
    # pass). When NOT provided, Engine.cpp leaves cameraOrbitActive=false
    # and World.inl's orbit override is skipped, falling back to the scene-
    # file Main Camera transform (R4 in the design pass).
    [string]$CameraOrbit = "",
    [string]$BinDir = "C:\GitRepo\InnocenceEngine\Bin\RelWithDebInfo",
    [string]$RunTag = ""
)

# TASK-213 CL C: the per-scene default. All three scenes share the same
# triple for now; per-scene overrides can be added if a scene needs a
# different framing once cross-binary determinism is closed.
$DefaultCameraOrbit = "20,8,120"

$ErrorActionPreference = "Stop"

Import-Module (Join-Path $PSScriptRoot 'Lib\Test-Engine.psm1') -Force

# Default the dump window to the back half of the run so accumulation has
# settled before the PNG sequence begins. Overridable via -DumpStart/-DumpEnd.
if ($DumpStart -lt 0) { $DumpStart = [int]($Frames / 2) }
if ($DumpEnd   -lt 0) { $DumpEnd   = $Frames - 1 }
if ($DumpEnd -lt $DumpStart) {
    Write-Host "FAIL - DumpEnd ($DumpEnd) < DumpStart ($DumpStart)."
    exit 1
}
if ($ExtraHeadroom -lt 0) {
    Write-Host "FAIL - ExtraHeadroom ($ExtraHeadroom) must be >= 0."
    exit 1
}
if ($FlapBackSafetyMargin -lt 0) {
    Write-Host "FAIL - FlapBackSafetyMargin ($FlapBackSafetyMargin) must be >= 0."
    exit 1
}

# TASK-213 CL D: engine-side capture window = user-requested + headroom.
# The script post-trims to the user's window (or a flap-back-shifted
# equivalent) after parsing the log. Clamp to $Frames-1 so the engine
# capture range never asks for frames past -total_frames.
$engineDumpEnd = [Math]::Min($DumpEnd + $ExtraHeadroom, $Frames - 1)
$dumpLen = $DumpEnd - $DumpStart + 1

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

    # TASK-213 CL D: capture wider than the user-requested window so the
    # post-run flap-back-aware shift has headroom (AC-7). Trimming happens
    # after the engine exits.
    $argList = "-renderer 0 -loglevel 0 -offscreen " +
               "-test gpu_path_tracer " +
               "-scene $($s.Scene) " +
               "-total_frames $Frames " +
               "-dump_frames $DumpStart-$engineDumpEnd"
    # TASK-213 CL C: resolve the effective orbit triple.
    #   user passed -CameraOrbit ""    -> per-scene default ($DefaultCameraOrbit)
    #   user passed -CameraOrbit "none" -> no -camera_orbit flag (scene-file camera)
    #   user passed -CameraOrbit "P,R,D" -> verbatim override
    if ($CameraOrbit -eq "") {
        $effectiveOrbit = $DefaultCameraOrbit
    } elseif ($CameraOrbit -eq "none") {
        $effectiveOrbit = ""
    } else {
        $effectiveOrbit = $CameraOrbit
    }
    if ($effectiveOrbit) {
        $argList += " -camera_orbit $effectiveOrbit"
    }

    $run = Invoke-EngineMainRun -BinDir $BinDir -ArgList $argList -NoNewWindow

    if (-not $run.LogFile) {
        Write-Host "FAIL [$($s.Name)] - No log file found."
        $overallPass = $false
        continue
    }

    $logFile = $run.LogFile
    $outcome = Test-EngineRunOutcome -LogFile $logFile `
        -SceneTag $s.SceneTag -ScenePrefix $s.Name

    # Steady-state grep enforcement: the engine must log
    # "Auto-test: steady state reached at frame=N" once per run
    # (FrameManagementServiceImpl.cpp). Without it the steady-state-relative
    # dump counter never starts and any captures are undefined.
    $steadyStateReached = Select-String -LiteralPath $logFile.FullName `
        -Pattern "Auto-test: steady state reached at frame=(\d+)"
    $steadyStateTimeout = Select-String -LiteralPath $logFile.FullName `
        -Pattern "Auto-test: steady state NOT reached within"
    # All flap-back markers (zero or more) — used by the dump-window shift below.
    $steadyStateLost = Select-String -LiteralPath $logFile.FullName `
        -Pattern "Auto-test: steady state lost at frame=(\d+)"

    Write-Host "Steady-state reached:      $($null -ne $steadyStateReached)"
    Write-Host "Steady-state timeout:      $($null -ne $steadyStateTimeout)"
    Write-Host "Steady-state lost events:  $(@($steadyStateLost).Count)"

    $scenePass = $outcome.Pass
    # Hard fails specific to the path-tracer capture contract — capture is
    # undefined without these.
    if (-not $steadyStateReached) {
        Write-Host "FAIL [$($s.Name)] - missing 'Auto-test: steady state reached at frame=N' log marker (TASK-213 AC-5). The engine never reached the readiness predicate; m_autoCaptureFrameCount never advanced; any captures are undefined. Diagnostic targets: deferred-init queue drain, TLAS rebuild loop, SceneService::IsLoading."
        $scenePass = $false
    }
    if ($steadyStateTimeout) {
        Write-Host "FAIL [$($s.Name)] - 120-frame steady-state timeout warning fired (TASK-213 R2). Capture determinism cannot be guaranteed; rerun with longer -total_frames or diagnose the deferred-init drain stall."
        $scenePass = $false
    }

    # TASK-213 CL D Workstream 2 (AC-7): flap-back-aware dump-window shift.
    # The K=3 latch can fire on a transient instance-count plateau; subsequent
    # rebuilds inside the dump window produce uniform-black readbacks. The
    # engine emits "Auto-test: steady state lost at frame=N" for each post-
    # latch flap-back. The "trustworthy" capture-frame is at least
    # FlapBackSafetyMargin past the last flap-back's auto-capture-frame index
    # (i.e. (L_max - S) + FlapBackSafetyMargin). If that threshold is past
    # the user-requested DumpStart, shift the post-trim window forward; if
    # it also exceeds the engine's wider capture range (DumpEnd +
    # ExtraHeadroom or $Frames-1), FAIL with option-(b) diagnostics so the
    # user can re-run with longer -total_frames or larger -ExtraHeadroom.
    $effectiveStart = $DumpStart
    $effectiveEnd = $DumpEnd
    $shiftReason = ""
    if ($steadyStateReached) {
        # NOTE: PowerShell variable names are case-insensitive, so $S would
        # collide with the foreach-loop variable $s holding the scene record.
        # Use $steadyFrame / $lastFlapFrame to avoid the clash.
        $steadyFrame = [int]$steadyStateReached.Matches[0].Groups[1].Value
        if ($steadyStateLost) {
            $lostFrames = @($steadyStateLost) | ForEach-Object { [int]$_.Matches[0].Groups[1].Value }
            $lastFlapFrame = ($lostFrames | Measure-Object -Maximum).Maximum
            # Threshold is in steady-state-relative (capture-frame) units.
            # Smallest acceptable capture-frame is the last-flap cap-frame
            # plus the safety margin (default 5: empirical settle margin
            # for DX12 frame-in-flight + PT accum reset, see param doc).
            $threshold = ($lastFlapFrame - $steadyFrame) + $FlapBackSafetyMargin
            if ($threshold -gt $effectiveStart) {
                $effectiveStart = $threshold
                $effectiveEnd = $threshold + ($dumpLen - 1)
                $shiftReason = "flap-back at FCSL=$lastFlapFrame (steady-state-relative cap-frame=$($lastFlapFrame - $steadyFrame)) past user DumpStart=$DumpStart; shifting to [$effectiveStart, $effectiveEnd]"
            }
        }
    }
    $captureCeiling = $engineDumpEnd
    if ($effectiveEnd -gt $captureCeiling) {
        Write-Host "FAIL [$($s.Name)] - flap-back pushed trustworthy dump end ($effectiveEnd) past captured-range ceiling ($captureCeiling). Rerun with longer -total_frames (currently $Frames) and/or larger -ExtraHeadroom (currently $ExtraHeadroom). Reason: $shiftReason"
        $scenePass = $false
    } elseif ($shiftReason) {
        Write-Host "INFO [$($s.Name)] - $shiftReason"
    } else {
        Write-Host "INFO [$($s.Name)] - no flap-back shift (effective window [$effectiveStart, $effectiveEnd])"
    }

    # Move ALL dumped PNGs into the per-scene capture directory first
    # (regardless of pass/fail — failure-case captures are diagnostic), then
    # delete those outside the trustworthy window so AC-1/AC-2 closure runs
    # see only frames the engine considers post-flap-back stable.
    # Engine writes to CWD = Bin/, not BinDir = Bin/RelWithDebInfo.
    $pngs = Get-ChildItem -Path $binRoot -Filter "gpu_output_*.png" -ErrorAction SilentlyContinue
    if ($pngs) {
        foreach ($png in $pngs) {
            Move-Item -LiteralPath $png.FullName -Destination $sceneOutDir -Force
        }
        Write-Host "Moved $($pngs.Count) capture(s) to $sceneOutDir"
    } else {
        Write-Host "WARN [$($s.Name)] - no gpu_output_*.png produced (engine dump range $DumpStart-$engineDumpEnd)."
    }

    # Trim PNGs outside the trustworthy [effectiveStart, effectiveEnd] window.
    # PNG filenames are gpu_output_NNNN.png where NNNN is the steady-state-
    # relative capture-frame index (CL B). Files outside the window are
    # moved to <sceneOutDir>/_trimmed/ rather than deleted, so a failed run
    # can still be diagnosed from the full capture set.
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
