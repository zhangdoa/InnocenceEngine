# HLSL -> DXIL compile wrapper (no-pause / automation variant).
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
# This script omits the trailing Pause from Scripts/HLSL2DXIL.ps1 so it can
# be invoked from CI / msbuild / hooks without blocking on user input.

param([switch]$FullClean)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$repoRoot   = Split-Path -Parent $scriptRoot

Import-Module (Join-Path $scriptRoot 'Lib\Compile-HLSL.psm1') -Force

Invoke-HlslToDxil `
    -HlslSourceDir (Join-Path $repoRoot 'Source\Shaders\HLSL') `
    -DxilOutputDir (Join-Path $repoRoot 'Bin\Shaders\DXIL') `
    -DxcExePath    (Join-Path $repoRoot 'Build\Tools\dxc\bin\x64\dxc.exe') `
    -FullClean:$FullClean
