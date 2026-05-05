# Build InnocenceEngine (Main + RenderTest).
#
# Uses inline msbuild invocation (not Start-Process) to avoid the stall where
# Start-Process -Wait keeps waiting after the top-level msbuild exits because
# its /m worker nodes inherited the redirected stdout/stderr handles.
# /nodeReuse:false ensures workers don't linger between invocations either.
#
# HLSL -> DXIL is compiled BEFORE msbuild every invocation. The compile pass
# is idempotent (Scripts/Lib/Compile-HLSL.psm1 does per-shader source-vs-DXIL
# LastWriteTimeUtc comparison and skips unchanged shaders, including
# #include-graph dependency checks), so a no-edit build pays only a directory
# enumeration. Without this, edits to .hlsl/.comp/.vert/.frag would silently
# leave the engine running stale DXIL — producing PSO E_INVALIDARG at runtime
# (see TASK-214 / TASK-213 CL C: HLSL edited 20:08, DXIL last compiled 20:03).
# Loud-fail with a "DXIL stale" diagnostic was the rejected alternative —
# would have required a second timestamp check in this script that drifts
# from the real compile policy. Rationale: Scripts/README.md.
#
# Pass -SkipShaderCompile to skip the shader pre-step (rare: bisecting a
# known-good DXIL set against a C++-only change). The default is to compile.

param(
    [switch]$SkipShaderCompile
)

$ErrorActionPreference = 'Stop'

$buildDir = Resolve-Path (Join-Path $PSScriptRoot '..\Build')
$outFile  = Join-Path $buildDir 'msbuild_out.txt'

function Invoke-MsBuild($project) {
    & msbuild.exe $project `
        /p:Configuration=RelWithDebInfo `
        /m `
        /nodeReuse:false `
        /nologo `
        /v:minimal `
        *>&1 | ForEach-Object {
            $line = $_.ToString()
            Write-Host $line
            Add-Content -Path $outFile -Value $line -Encoding ASCII
        }
    return $LASTEXITCODE
}

# Clear log so error greps don't match stale output from a previous build.
Set-Content -Path $outFile -Value '' -Encoding ASCII

# HLSL -> DXIL pre-step. Idempotent: the underlying module skips shaders
# whose .dxil is newer than both the source and every #include dependency.
# A failure here aborts the build — proceeding to msbuild with a partial
# DXIL set would just reproduce the silent-stale failure mode in a new
# disguise (some PSOs build, others E_INVALIDARG at runtime).
if ($SkipShaderCompile) {
    Write-Host '[BuildWin] -SkipShaderCompile set — skipping HLSL -> DXIL pre-step.'
} else {
    Write-Host '[BuildWin] HLSL -> DXIL pre-step (idempotent; recompiles only stale shaders)...'
    & (Join-Path $PSScriptRoot 'HLSL2DXIL_NoPause.ps1')
    if ($LASTEXITCODE -ne 0) {
        Write-Error "[BuildWin] HLSL -> DXIL pre-step failed (exit $LASTEXITCODE). Aborting build."
        exit $LASTEXITCODE
    }
}

Push-Location $buildDir
try {
    $mainProj = Join-Path $buildDir 'Source\Engine\Platform\WinMain\Main.vcxproj'
    $testProj = Join-Path $buildDir 'Source\Engine\Platform\WinMain\RenderTest.vcxproj'

    $mainExit = Invoke-MsBuild $mainProj
    $testExit = Invoke-MsBuild $testProj

    $exitCode = if ($mainExit -ne 0) { $mainExit } else { $testExit }
}
finally {
    Pop-Location
}

Get-Content $outFile | Select-Object -Last 5
exit $exitCode
