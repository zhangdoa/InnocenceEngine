# Install the tracked git hooks under .git/hooks/ via copy.
#
# Why install (not core.hooksPath)
# --------------------------------
# core.hooksPath is per-clone configuration that this script could set,
# but the tracked hooks need executable bits on Unix and Windows treats
# all files as executable -- so a plain copy works on every host without
# extra config. Copy-once-and-forget; re-run after pulling new hooks.
#
# What gets installed
# -------------------
# - post-checkout: purge stale clangd index after branch / file checkout.
# - post-merge:    same, after merge / pull.
# Both are TASK-151 mirror-semantic hygiene. See Scripts/git-hooks/*.
#
# Re-run when
# -----------
# - After cloning (.git/hooks/ starts empty except for *.sample).
# - After Scripts/git-hooks/* is updated (pulls do not propagate to
#   .git/hooks/ automatically).

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot     = Resolve-Path (Join-Path $PSScriptRoot '../..')
$dstHooksDir  = Join-Path $repoRoot.Path '.git/hooks'

if (-not (Test-Path $dstHooksDir)) {
    Write-Error "git hooks dir not found: $dstHooksDir (is $($repoRoot.Path) a git checkout?)"
    exit 1
}

# Copy every file in Scripts/git-hooks/ except this installer itself.
$hookFiles = Get-ChildItem -Path $PSScriptRoot -File | Where-Object {
    $_.Name -ne 'InstallHooks.ps1'
}

if ($hookFiles.Count -eq 0) {
    Write-Host "No hook files to install in $PSScriptRoot."
    exit 0
}

foreach ($hook in $hookFiles) {
    $dst = Join-Path $dstHooksDir $hook.Name
    Copy-Item -LiteralPath $hook.FullName -Destination $dst -Force
    Write-Host "Installed: $($hook.Name) -> $dst"
}

Write-Host ""
Write-Host "Done. $($hookFiles.Count) hook(s) installed under $dstHooksDir."
