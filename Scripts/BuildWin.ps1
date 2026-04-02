$buildDir = Join-Path $PSScriptRoot '..\Build'
$outFile  = Join-Path $buildDir 'msbuild_out.txt'
$errFile  = Join-Path $buildDir 'msbuild_err.txt'

# Build Main (primary — all shared libs are pulled in as dependencies)
$mainProj = Join-Path $buildDir 'Source\Engine\Platform\WinMain\Main.vcxproj'
$proc = Start-Process -FilePath 'msbuild.exe' `
    -ArgumentList "$mainProj /p:Configuration=RelWithDebInfo /m /nologo /v:minimal" `
    -WorkingDirectory $buildDir `
    -Wait -PassThru `
    -RedirectStandardOutput $outFile `
    -RedirectStandardError $errFile
$exitCode = $proc.ExitCode

# Build RenderTest (links against the same libs; only changed files recompile)
$testProj = Join-Path $buildDir 'Source\Engine\Platform\WinMain\RenderTest.vcxproj'
$testOut  = Join-Path $buildDir 'msbuild_rendertest_out.txt'
$proc2 = Start-Process -FilePath 'msbuild.exe' `
    -ArgumentList "$testProj /p:Configuration=RelWithDebInfo /m /nologo /v:minimal" `
    -WorkingDirectory $buildDir `
    -Wait -PassThru `
    -RedirectStandardOutput $testOut `
    -RedirectStandardError $errFile
if ($proc2.ExitCode -ne 0) { $exitCode = $proc2.ExitCode }

# Append RenderTest output so error greps cover both
Get-Content $testOut | Add-Content $outFile

Get-Content $outFile | Select-Object -Last 5
exit $exitCode
