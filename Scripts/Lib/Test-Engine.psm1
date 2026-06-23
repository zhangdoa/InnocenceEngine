# Test-Engine.psm1
#
# Shared Main.exe-driver boilerplate for the three engine smoke-test scripts
# (TestGIScene.ps1, TestPT.ps1, TestPTThreeScenes.ps1).
#
# Each driver script flips Main.exe into headless / offscreen mode, waits for
# auto-termination, then asserts on three universal post-run predicates:
#
#   1. No "D3D12 ERROR" / "CORRUPTION" / "Validation Error" lines in the log.
#   2. The expected scene was loaded (".InnoScene has been loaded.").
#   3. The engine auto-terminated ("Auto-test: ... terminating.").
#
# These predicates and the log-discovery / CWD shape are the single source of
# truth; per-driver MAE / steady-state / capture logic stays in each caller.
#
# INVARIANTS shared across all three drivers:
#   - Engine writes its rolling log to <BinRoot>\*.Log (NOT BinDir = the
#     RelWithDebInfo subdir). Main.exe's IOService resolves paths relative
#     to its CWD = <BinRoot>, not its exe directory.
#   - -loglevel must be <= 1 (Success). The success markers grep'd here are
#     emitted at LogLevel::Success (Source/Engine/Common/LogService.h);
#     -loglevel 2 hides them and the script reports a false FAIL.
#   - -total_frames is the auto-terminate budget (parsed in
#     Source/Engine/Engine.cpp). -frames is silently ignored.
#   - Auto-terminate marker wording is locked to "Auto-test: ... terminating"
#     in Engine.cpp; scene-loaded marker is "<Scene> has been loaded" in
#     World.inl. If either changes, update both sides.
#
# Invoke-EngineBounded is the programmatic-verification launcher: it kills
# stragglers + settles before launch, bounds the intermittent TASK-241
# init/shutdown hang via WaitForExit + Kill (never orphaning the child).
#
# RELIABLE SIGNALS for an offscreen run (observed 2026-06-23): the process
# EXIT CODE and the PRODUCED ARTIFACTS - NOT the text logs. Main.exe is a
# /SUBSYSTEM:WINDOWS (WinMain) binary: it writes nothing to a redirected or
# inherited stdout, AND its timestamped *.Log is empty in offscreen mode
# (LogService flushes the file only on a graceful interactive dtor). A full
# healthy audit run can leave BOTH stdout and *.Log at 0 bytes. So:
#   - PASS gate = (not TimedOut) + expected artifacts present at non-degenerate
#     size (Test-EngineArtifacts), confirmed by exit code 0 when available.
#   - The log-grep predicates (Test-EngineRunOutcome) are meaningful only when
#     the log is non-empty (windowed runs); on an empty log they are
#     INCONCLUSIVE (LogEmpty=$true), not a FAIL.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-EngineMainRun {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$BinDir,
        [Parameter(Mandatory)] [string]$ArgList,
        [switch]                       $NoNewWindow
    )

    $mainExe = Join-Path $BinDir 'Main.exe'
    if (-not (Test-Path $mainExe)) {
        throw "Main.exe not found at $mainExe"
    }

    # Bin/ (the parent of BinDir = Bin/RelWithDebInfo) is both the CWD for
    # Main.exe and the directory IOService writes log + capture output into.
    $binRoot = Split-Path $BinDir -Parent
    Set-Location $binRoot

    Write-Host "Running: $mainExe $ArgList"

    $startArgs = @{
        FilePath     = $mainExe
        ArgumentList = $ArgList
        Wait         = $true
        PassThru     = $true
    }
    if ($NoNewWindow) { $startArgs['NoNewWindow'] = $true }

    $proc = Start-Process @startArgs
    Write-Host "Exit code: $($proc.ExitCode)"

    $logFile = Get-ChildItem (Join-Path $binRoot '*.Log') |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    [PSCustomObject]@{
        Process = $proc
        LogFile = $logFile
        BinRoot = $binRoot
    }
}

function Test-EngineRunOutcome {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [System.IO.FileInfo]$LogFile,
        [Parameter(Mandatory)] [string]            $SceneTag,
        [string]                                   $ScenePrefix = ''
    )

    # SceneTag: the literal substring grep'd against the load marker
    # ("<SceneTag> has been loaded"). ScenePrefix: optional bracket tag for
    # the per-scene-loop driver to disambiguate output ("[unittest] - ...").

    Write-Host "Log: $($LogFile.Name)"

    $logEmpty = ((Get-Item -LiteralPath $LogFile.FullName).Length -eq 0)
    if ($logEmpty) {
        Write-Host "WARN - log is empty (offscreen run; LogService does not flush *.Log here)."
        Write-Host "       Load/terminate predicates are INCONCLUSIVE; gate on exit code + artifacts."
    }

    # Coerce all Select-String results to arrays so the .Count / .Length /
    # foreach calls below stay valid under Set-StrictMode -Version Latest
    # (a no-match Select-String returns $null, not an empty collection).
    $d3dErrors = @(Select-String -LiteralPath $LogFile.FullName `
        -Pattern 'D3D12 ERROR|CORRUPTION|Validation Error' -SimpleMatch)

    $sceneLoaded = @(Select-String -LiteralPath $LogFile.FullName `
        -Pattern "$SceneTag has been loaded" -SimpleMatch)

    $autoTerminated = @(Select-String -LiteralPath $LogFile.FullName `
        -Pattern 'Auto-test:.*terminating')

    Write-Host "$SceneTag loaded:  $($sceneLoaded.Count -gt 0)"
    Write-Host "Auto-terminated:  $($autoTerminated.Count -gt 0)"
    Write-Host "D3D12 errors:     $($d3dErrors.Count)"

    $tag = if ($ScenePrefix) { " [$ScenePrefix]" } else { '' }
    $pass = $true

    if ($d3dErrors.Count -gt 0) {
        Write-Host "FAIL$tag - D3D12 errors detected:"
        $d3dErrors | ForEach-Object { Write-Host "  $_" }
        $pass = $false
    }
    # Scene-load / auto-terminate are only assertable from a populated log.
    # On an empty offscreen log they are inconclusive, never a hard FAIL -
    # the caller must gate on exit code + Test-EngineArtifacts instead.
    if (-not $logEmpty) {
        if ($sceneLoaded.Count -eq 0) {
            Write-Host "FAIL$tag - $SceneTag was not loaded."
            $pass = $false
        }
        if ($autoTerminated.Count -eq 0) {
            Write-Host "FAIL$tag - engine did not auto-terminate (crashed or hung?)."
            $pass = $false
        }
    }

    [PSCustomObject]@{
        Pass           = $pass
        LogEmpty       = $logEmpty
        D3DErrors      = $d3dErrors
        SceneLoaded    = $sceneLoaded
        AutoTerminated = $autoTerminated
    }
}

function Invoke-EngineBounded {
    # Bounded launch for programmatic verification. Three lessons baked in:
    #   1. Kill stragglers + settle first - a run killed mid-frame leaves the virtual
    #      GPU / swap-chain in a state that hangs the next launch (TASK-241).
    #   2. WaitForExit(timeout) then Kill the child directly - NEVER orphan it. A bash
    #      `timeout` wrapping this script kills PowerShell but leaves Main.exe running
    #      and GPU-locked, which then hangs every subsequent run.
    #   3. Capture stdout/stderr to files - but note BOTH these and the *.Log
    #      are typically empty offscreen (see file header); verify via exit
    #      code + artifacts, with the captures only a best-effort crash hint.
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$BinDir,
        [Parameter(Mandatory)] [string]$ArgList,
        [int]$TimeoutSec = 120,
        [int]$SettleSec  = 4
    )

    Get-Process -Name Main, RenderTest -ErrorAction SilentlyContinue | ForEach-Object { $_.Kill() }
    if ($SettleSec -gt 0) { Start-Sleep -Seconds $SettleSec }

    $mainExe = Join-Path $BinDir 'Main.exe'
    if (-not (Test-Path $mainExe)) { throw "Main.exe not found at $mainExe" }

    $binRoot = Split-Path $BinDir -Parent
    Set-Location $binRoot
    $stdout = Join-Path $binRoot 'engine_stdout.txt'
    $stderr = Join-Path $binRoot 'engine_stderr.txt'

    Write-Host "Running (bounded ${TimeoutSec}s): $mainExe $ArgList"
    $proc = Start-Process -FilePath $mainExe -ArgumentList $ArgList `
        -WorkingDirectory $binRoot -PassThru -NoNewWindow `
        -RedirectStandardOutput $stdout -RedirectStandardError $stderr

    $exited = $proc.WaitForExit($TimeoutSec * 1000)
    if (-not $exited) {
        Write-Host "TIMEOUT after ${TimeoutSec}s - killing (likely TASK-241 init/shutdown hang)."
        $proc.Kill()
        Start-Sleep -Seconds 1
        $exitCode = $null
    } else {
        # WaitForExit(timeout) can return before ExitCode is populated; the
        # no-arg call reaps fully. Guard the read - a fast-exiting child whose
        # handle was already closed throws on .ExitCode.
        try { $proc.WaitForExit() } catch {}
        try { $exitCode = $proc.ExitCode } catch { $exitCode = $null }
        Write-Host "Exit code: $exitCode"
    }

    [PSCustomObject]@{
        Process  = $proc
        TimedOut = (-not $exited)
        ExitCode = $exitCode
        Stdout   = $stdout
        Stderr   = $stderr
        BinRoot  = $binRoot
    }
}

function Test-EngineArtifacts {
    # Exit-code-/log-independent verification for offscreen runs: assert the
    # run PRODUCED the expected artifacts at a non-degenerate size. This is the
    # reliable signal when stdout and *.Log are both empty (see file header).
    #
    #   -Path     SPECIFIC scene-render artifacts, each must resolve to >=1 file
    #             - the GBuffer / LightPass / FinalBlend passes that CANNOT be
    #             black on a healthy render (e.g. audit_00a_OpaquePass_RT_0,
    #             audit_08_LightPass_Luminance, audit_13_FinalBlend). Do NOT
    #             glob all audit_*.hdr: LUTs and masks (BRDFLUTMS ~22KB,
    #             SSRC ProbeMask ~2KB) are LEGITIMATELY small/uniform and would
    #             false-fail the size floor (verified 2026-06-23).
    #   -MinBytes per-file floor. A collapsed / all-black / uniform 720p render
    #             RLE-compresses far below this; real scene-render HDRs were
    #             280-666 KB this session, so the 50KB default separates cleanly.
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string[]]$Path,
        [int]$MinBytes = 51200,
        [string]$Label = ''
    )
    $tag = if ($Label) { " [$Label]" } else { '' }
    $missing  = @()
    $tooSmall = @()
    $okCount  = 0
    foreach ($p in $Path) {
        $hits = @(Get-ChildItem -Path $p -File -ErrorAction SilentlyContinue)
        if ($hits.Count -eq 0) { $missing += $p; continue }
        foreach ($f in $hits) {
            if ($f.Length -lt $MinBytes) {
                $tooSmall += ("{0} ({1} B)" -f $f.Name, $f.Length)
            } else {
                $okCount++
            }
        }
    }
    $pass = ($missing.Count -eq 0) -and ($tooSmall.Count -eq 0)
    Write-Host ("artifacts${tag}: ok={0} missing={1} degenerate={2} (floor {3} B)" -f `
        $okCount, $missing.Count, $tooSmall.Count, $MinBytes)
    if ($missing.Count)  { Write-Host "  MISSING:    $($missing -join ', ')" }
    if ($tooSmall.Count) { Write-Host "  DEGENERATE: $($tooSmall -join ', ')" }
    if (-not $pass) { Write-Host "FAIL$tag - expected artifacts not produced / degenerate." }

    [PSCustomObject]@{
        Pass       = $pass
        OkCount    = $okCount
        Missing    = $missing
        Degenerate = $tooSmall
    }
}

Export-ModuleMember -Function Invoke-EngineMainRun, Invoke-EngineBounded, Test-EngineRunOutcome, Test-EngineArtifacts
