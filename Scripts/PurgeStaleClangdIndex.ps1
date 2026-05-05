# Purge stale clangd index entries whose primary source file no longer
# exists on disk. Mirrors the orphan-DXIL purge in Scripts/Lib/Compile-HLSL.psm1
# (TASK-146): the local cache must reflect the current source tree, not a
# union of the source tree and every revert/branch-switch the developer ever
# performed.
#
# Why this exists (TASK-151)
# --------------------------
# clangd persists per-translation-unit index files at .cache/clangd/index/
# named "<source-basename>.<path-hash>.idx". When a source file is deleted
# (revert of WIP, branch switch, bisect step) the .idx file lingers and its
# symbols continue to participate in cross-TU lookups. Visible symptoms:
#
#   - Ghost "'<file>.h' file not found" on tracked files that don't include it.
#   - Ghost "Use of undeclared identifier 'X'" at lines that don't reference X.
#   - False inheritance errors on tracked classes that cleanly inherit their
#     declared base.
#
# Real cmake --build is unaffected; this is purely IDE noise. But the noise
# wastes triage time on every session start after a deletion-bearing checkout,
# and per feedback_no_dismissing_tool_noise.md the team fixes the source
# instead of learning to ignore it.
#
# Mechanism
# ---------
# Each .idx is a binary blob, but it contains plain ASCII "file:///..." URIs
# for every translation-unit input. The .idx filename is "<basename>.<hash>.idx";
# the URI whose path basename matches "<basename>" is the primary source file
# the index represents. If that path no longer exists on disk, the .idx is
# unambiguously orphan and gets deleted.
#
# What we deliberately do NOT do
# ------------------------------
# - We do not regenerate compile_commands.json (that's a heavier step and only
#   needed when CMake / include paths change; see Scripts/RegenClangdIndex.ps1).
# - We do not touch system-header / third-party-header indices that have no
#   primary-source URI in the project tree -- leaving them alone preserves
#   correct cross-references to <vector>, <Windows.h>, etc.
# - We do not parse the .idx binary format beyond ASCII-grepping the URIs.
#   The format is unstable across clangd versions; URI strings are the most
#   durable signal.

[CmdletBinding()]
param(
    [string]$IndexDir,
    [switch]$DryRun,
    [switch]$ShowEachIdx,
    [switch]$PurgeAll
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Resolve default IndexDir relative to the script location, not the caller's
# cwd. We do this in the body (not as a param default) so $PSScriptRoot is
# always populated -- some invocation paths leave it empty inside param().
if ([string]::IsNullOrEmpty($IndexDir)) {
    if ([string]::IsNullOrEmpty($PSScriptRoot)) {
        # Last-resort fallback when invoked via a path that doesn't set
        # $PSScriptRoot (rare; typically only the dot-source-from-stdin case).
        $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    } else {
        $scriptDir = $PSScriptRoot
    }
    $IndexDir = Join-Path $scriptDir '../.cache/clangd/index'
}

if (-not (Test-Path $IndexDir)) {
    Write-Host "Clangd index dir not found: $IndexDir (nothing to purge)."
    exit 0
}

# -PurgeAll: nuclear reset for the rare stale-but-live-source case
# (TASK-199). The orphan-source heuristic below cannot detect a TU whose
# source still exists but whose cached preprocessor / token state is
# corrupt -- the symptom we observed in Engine.cpp where clangd reported
# "Pasting formed '<HIDService' invalid preprocessing token" on lines
# that compile cleanly. Mtime comparison cannot catch this either: the
# .idx is newer than the source, but its contents are wrong.
#
# Recovery is to drop every .idx and let clangd rebuild from scratch in
# the background. Costs minutes of CPU off the user's interactive path;
# correctness benefit is deterministic. The orphan-source heuristic is
# left in place for the cheap case below; -PurgeAll is the escape hatch
# documented in Scripts/README.md.
if ($PurgeAll) {
    $indexDirResolved = (Resolve-Path $IndexDir).Path
    $idxFiles = Get-ChildItem -LiteralPath $indexDirResolved -File -Filter '*.idx'
    $count = $idxFiles.Count
    $bytes = ($idxFiles | Measure-Object -Property Length -Sum).Sum
    if (-not $bytes) { $bytes = 0 }
    $kb = [Math]::Round($bytes / 1KB, 1)
    if ($DryRun) {
        Write-Host "[dry-run] -PurgeAll would delete $count .idx files ($kb KB) from $indexDirResolved"
    } else {
        foreach ($idx in $idxFiles) {
            Remove-Item -LiteralPath $idx.FullName -Force
        }
        Write-Host "Purged $count .idx files ($kb KB) from $indexDirResolved (-PurgeAll)."
    }
    exit 0
}

$indexDirResolved = (Resolve-Path $IndexDir).Path
$idxFiles = Get-ChildItem -LiteralPath $indexDirResolved -File -Filter '*.idx'

if ($idxFiles.Count -eq 0) {
    Write-Host "No .idx files in $indexDirResolved -- nothing to purge."
    exit 0
}

# Pattern matches "<basename>.<16-hex-chars>.idx" -- clangd's per-TU naming.
# Anything else in the dir we leave alone (defensive: unknown clangd version
# might emit different shapes).
$idxNamePattern = '^(?<base>.+)\.[0-9A-F]{16}\.idx$'

$total       = 0
$orphan      = 0
$kept        = 0
$skipped     = 0
$bytesFreed  = 0L

foreach ($idx in $idxFiles) {
    $total++

    $match = [regex]::Match($idx.Name, $idxNamePattern)
    if (-not $match.Success) {
        if ($ShowEachIdx) {
            Write-Host "  [skip] unrecognized idx name shape: $($idx.Name)"
        }
        $skipped++
        continue
    }

    $sourceBasename = $match.Groups['base'].Value

    # Read .idx as bytes, project printable ASCII into a single string,
    # extract every "file:///..." URI. clangd encodes paths as forward-slash
    # URIs even on Windows.
    $bytes = [System.IO.File]::ReadAllBytes($idx.FullName)
    $sb = [System.Text.StringBuilder]::new($bytes.Length)
    foreach ($b in $bytes) {
        if ($b -ge 32 -and $b -lt 127) {
            [void]$sb.Append([char]$b)
        } else {
            [void]$sb.Append(' ')
        }
    }
    $printable = $sb.ToString()

    # Find the URI whose basename matches the .idx's source-basename. That's
    # the primary source path for this index. Anchor the match on "/<base>"
    # followed by a non-path char to avoid matching e.g. "Foo.cpp" inside
    # "Foo.cpp.bak".
    $primaryPattern = "file:///[^ ]*/$([regex]::Escape($sourceBasename))(?=[ ?#]|$)"
    $primaryMatch   = [regex]::Match($printable, $primaryPattern)

    if (-not $primaryMatch.Success) {
        # No URI matches the .idx basename. Two real cases:
        #  (a) clangd indexed a virtual / built-in TU with no on-disk URI;
        #  (b) the .idx was truncated / corrupt.
        # Either way we keep it -- purging here would delete legitimate cache.
        if ($ShowEachIdx) {
            Write-Host "  [keep] $($idx.Name) -- no primary URI for '$sourceBasename'"
        }
        $kept++
        continue
    }

    # Convert "file:///C:/foo/bar.h" -> "C:\foo\bar.h" for Test-Path.
    $uri        = $primaryMatch.Value
    $sourcePath = $uri -replace '^file:///',''
    $sourcePath = $sourcePath -replace '/','\'
    # URIs may contain percent-encoded chars (spaces in paths etc.); decode.
    $sourcePath = [System.Uri]::UnescapeDataString($sourcePath)

    if (Test-Path -LiteralPath $sourcePath) {
        $kept++
        if ($ShowEachIdx) {
            Write-Host "  [keep] $($idx.Name) -> $sourcePath"
        }
        continue
    }

    $orphan++
    $bytesFreed += $idx.Length
    if ($DryRun) {
        Write-Host "[dry-run] would purge orphan: $($idx.Name) (primary src absent: $sourcePath)"
    } else {
        Write-Host "Purging orphan idx: $($idx.Name) (primary src absent: $sourcePath)"
        Remove-Item -LiteralPath $idx.FullName -Force
    }
}

$kbFreed = [Math]::Round($bytesFreed / 1KB, 1)
$action  = if ($DryRun) { 'would purge' } else { 'purged' }
Write-Host ""
Write-Host "Clangd index purge summary:"
Write-Host ("  scanned:           {0}" -f $total)
Write-Host ("  {0}:        {1} ({2} KB)" -f $action, $orphan, $kbFreed)
Write-Host ("  kept (live):       {0}" -f $kept)
if ($skipped -gt 0) {
    Write-Host ("  skipped (shape):   {0}" -f $skipped)
}
