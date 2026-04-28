# Compile-HLSL.psm1
#
# Single source of truth for the HLSL -> DXIL compile step. Wrappers in
# Scripts/HLSL2DXIL.ps1 and Scripts/HLSL2DXIL_NoPause.ps1 delegate here.
#
# MIRROR SEMANTICS — TASK-146
# ---------------------------
# The DXIL output directory must EXACTLY mirror the set of compiled-shader
# source files. Stale .dxil files left over from deleted/renamed source shaders
# (revert of WIP, branch switch, bisect step) caused the TASK-141..145 phantom
# regression chain: an older bisect commit's engine binary loaded a newer
# commit's DXIL with mismatched bindings -> device-removed -> hours wasted on
# a non-existent "shadow regression."
#
# This module guarantees mirror semantics by enumerating expected DXIL outputs
# from the source tree and deleting any orphan .dxil that does not have a
# corresponding source. The orphan delete pass runs BEFORE compile so removed
# shaders are gone from the cache even on a no-op incremental build.
#
# A -FullClean switch additionally wipes the entire DXIL output directory at
# start; useful for paranoid bisect steps where any cache trust is unwanted.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Per-file profile overrides. Keyed by source filename; consulted before the
# extension-based default. Use when a single shader needs a higher SM than its
# siblings (e.g. inline RayQuery requires SM 6.5 / DXR Tier 1.1).
$script:ShaderProfileOverrides = @{
    'lightPass.comp' = 'cs_6_5'  # TASK-176: inline RayQuery for unified RT shadows.
}

function Get-ShaderTargetProfile {
    param([System.IO.FileInfo]$File)

    if ($script:ShaderProfileOverrides.ContainsKey($File.Name)) {
        return $script:ShaderProfileOverrides[$File.Name]
    }

    switch ($File.Extension) {
        '.vert' { return 'vs_6_3' }
        '.frag' { return 'ps_6_3' }
        '.tesc' { return 'hs_6_3' }
        '.tese' { return 'ds_6_3' }
        '.geom' { return 'gs_6_3' }
        '.comp' { return 'cs_6_3' }
        default { return 'lib_6_3' } # Any .hlsl is a DXR library by convention.
    }
}

function Get-ShaderEntryPoint {
    param([System.IO.FileInfo]$File)

    if ($File.Extension -ne '.hlsl') { return 'main' }

    if ($File.Name -match 'RayGen')     { return 'RayGenShader' }
    if ($File.Name -match 'ClosestHit') { return 'ClosestHitShader' }
    if ($File.Name -match 'AnyHit')     { return 'AnyHitShader' }
    if ($File.Name -match 'Miss')       { return 'MissShader' }
    return 'main'
}

function Get-ShaderSourceFiles {
    param([string]$HlslSourceDir)

    # Top-level only. common/ holds shared #include headers (not standalone
    # compile units); WIP/ holds experimental files. Both deliberately excluded
    # by not recursing.
    Get-ChildItem -Path $HlslSourceDir -File | Where-Object {
        $_.Extension -eq '.hlsl' -or
        $_.Extension -match '\.(vert|frag|tesc|tese|geom|comp)$'
    }
}

function Remove-OrphanDxil {
    param(
        [Parameter(Mandatory)] [string]   $DxilOutputDir,
        [Parameter(Mandatory)] [hashtable]$ExpectedDxilNames  # name -> $true
    )

    if (-not (Test-Path $DxilOutputDir)) { return 0 }

    $orphans = Get-ChildItem -Path $DxilOutputDir -File -Filter '*.dxil' |
        Where-Object { -not $ExpectedDxilNames.ContainsKey($_.Name) }

    foreach ($orphan in $orphans) {
        Write-Host "Removing orphan DXIL (no matching source): $($orphan.Name)"
        Remove-Item -LiteralPath $orphan.FullName -Force
    }

    return @($orphans).Count
}

function Invoke-HlslToDxil {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$HlslSourceDir,
        [Parameter(Mandatory)] [string]$DxilOutputDir,
        [Parameter(Mandatory)] [string]$DxcExePath,
        [switch]                       $FullClean   # Nuke DXIL dir before compile.
    )

    if (-not (Test-Path $DxcExePath)) {
        throw "dxc.exe not found at $DxcExePath"
    }
    if (-not (Test-Path $HlslSourceDir)) {
        throw "HLSL source directory not found: $HlslSourceDir"
    }

    if ($FullClean -and (Test-Path $DxilOutputDir)) {
        Write-Host "[-FullClean] Wiping DXIL output directory: $DxilOutputDir"
        Remove-Item -LiteralPath $DxilOutputDir -Recurse -Force
    }

    if (-not (Test-Path $DxilOutputDir)) {
        Write-Host "Creating DXIL output directory: $DxilOutputDir"
        New-Item -ItemType Directory -Path $DxilOutputDir | Out-Null
    }

    Write-Host "Collecting shaders from $HlslSourceDir..."
    $shaderFiles = Get-ShaderSourceFiles -HlslSourceDir $HlslSourceDir

    if ($shaderFiles.Count -eq 0) {
        Write-Host "No shader files found in $HlslSourceDir. Exiting."
        return
    }

    # Mirror-semantic step: build the expected-DXIL set and delete orphans
    # BEFORE compile. Naming convention: <shader>.<ext>.dxil (preserves the
    # source extension so .vert/.frag/.comp/.hlsl all coexist without collision).
    $expectedDxilNames = @{}
    foreach ($file in $shaderFiles) {
        $expectedDxilNames["$($file.Name).dxil"] = $true
    }

    $orphanCount = Remove-OrphanDxil `
        -DxilOutputDir $DxilOutputDir `
        -ExpectedDxilNames $expectedDxilNames

    if ($orphanCount -gt 0) {
        Write-Host "Removed $orphanCount orphan DXIL file(s)."
    }

    # Build dependency map for #include-driven recompiles.
    $shaderDependencies = @{}
    foreach ($file in $shaderFiles) {
        $shaderDependencies[$file.Name] = @()
        foreach ($line in (Get-Content -Path $file.FullName)) {
            if ($line -match '^\s*#include\s*"(.+?)"') {
                $shaderDependencies[$file.Name] += $matches[1]
            }
        }
    }

    $commonArgs = @('-Wno-ignored-attributes', '-Qembed_debug', '/Zi', '/Zss')

    foreach ($file in $shaderFiles) {
        $sourcePath = $file.FullName
        $outputPath = Join-Path $DxilOutputDir "$($file.Name).dxil"

        $targetProfile = Get-ShaderTargetProfile -File $file
        $entryPoint    = Get-ShaderEntryPoint    -File $file

        $recompile = $true
        if (Test-Path $outputPath) {
            $sourceTime = (Get-Item $sourcePath).LastWriteTimeUtc
            $outputTime = (Get-Item $outputPath).LastWriteTimeUtc

            if ($sourceTime -le $outputTime) { $recompile = $false }

            foreach ($includedFile in $shaderDependencies[$file.Name]) {
                $includedPath = Join-Path $HlslSourceDir $includedFile
                if (Test-Path $includedPath) {
                    if ((Get-Item $includedPath).LastWriteTimeUtc -gt $outputTime) {
                        Write-Host "Dependency changed: $includedFile -> Recompiling $($file.Name)"
                        $recompile = $true
                    }
                }
            }
        }

        if (-not $recompile) {
            Write-Host "Skipping unchanged shader: $($file.Name)"
            continue
        }

        $dxcArgs = @(
            '-T', $targetProfile,
            '-E', $entryPoint,
            '-Fo', $outputPath,
            $sourcePath
        ) + $commonArgs

        Write-Host "Compiling $($file.Name) -> $outputPath (Profile: $targetProfile)"

        $process = Start-Process -FilePath $DxcExePath `
            -ArgumentList $dxcArgs -NoNewWindow -Wait -PassThru

        if ($process.ExitCode -ne 0) {
            Write-Error "Error compiling $($file.Name). Exit code: $($process.ExitCode)"
        } else {
            Write-Host "Successfully compiled $($file.Name)."
        }
    }

    Write-Host "Shader compilation complete."
}

Export-ModuleMember -Function Invoke-HlslToDxil
