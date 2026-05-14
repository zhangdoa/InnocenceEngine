# HLSL -> DXIL compile wrapper.
#
# Thin entry point that delegates to Scripts/Lib/Compile-HLSL.psm1. The shared
# module enforces mirror semantics on Bin/Shaders/DXIL/ — orphan .dxil files
# (whose source HLSL no longer exists) are deleted before compile, so a
# branch switch / revert / bisect step never leaves stale binaries that an
# engine binary could load with mismatched bindings. See TASK-146 for the
# regression chain that motivated this rule.
#
# Pass -FullClean to additionally wipe the entire output directory at start
# (forces full rebuild; useful for paranoid bisects).
#
# Pass -NoPause to suppress the trailing Pause; required for CI / msbuild /
# CMake / git-hook invocations that must not block on user input. The default
# (no switch) keeps the Pause for interactive double-click usage. TASK-212
# Phase 2 consolidated the previously-split HLSL2DXIL_NoPause.ps1 into this
# switch.

param(
    [switch]$FullClean,
    [switch]$NoPause
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$repoRoot   = Split-Path -Parent $scriptRoot

Import-Module (Join-Path $scriptRoot 'Lib\Compile-HLSL.psm1') -Force

Invoke-HlslToDxil `
    -HlslSourceDir (Join-Path $repoRoot 'Source\Shaders\HLSL') `
    -DxilOutputDir (Join-Path $repoRoot 'Bin\Shaders\DXIL') `
    -DxcExePath    (Join-Path $repoRoot 'Build\Tools\dxc\bin\x64\dxc.exe') `
    -FullClean:$FullClean

if (-not $NoPause) { Pause }
