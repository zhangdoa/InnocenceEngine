# Build InnocenceEngine (Main + RenderTest).
#
# Uses inline msbuild invocation (not Start-Process) to avoid the stall where
# Start-Process -Wait keeps waiting after the top-level msbuild exits because
# its /m worker nodes inherited the redirected stdout/stderr handles.
# /nodeReuse:false ensures workers don't linger between invocations either.

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
