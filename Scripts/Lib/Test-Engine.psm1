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
    if ($sceneLoaded.Count -eq 0) {
        Write-Host "FAIL$tag - $SceneTag was not loaded."
        $pass = $false
    }
    if ($autoTerminated.Count -eq 0) {
        Write-Host "FAIL$tag - engine did not auto-terminate (crashed or hung?)."
        $pass = $false
    }

    [PSCustomObject]@{
        Pass           = $pass
        D3DErrors      = $d3dErrors
        SceneLoaded    = $sceneLoaded
        AutoTerminated = $autoTerminated
    }
}

Export-ModuleMember -Function Invoke-EngineMainRun, Test-EngineRunOutcome
