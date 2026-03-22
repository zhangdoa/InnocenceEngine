$buildDir = Join-Path $PSScriptRoot '..\Build'
$proj = Join-Path $buildDir 'Source\Engine\Platform\WinMain\Main.vcxproj'
$outFile = Join-Path $buildDir 'msbuild_out.txt'
$errFile = Join-Path $buildDir 'msbuild_err.txt'

$proc = Start-Process -FilePath 'msbuild.exe' `
    -ArgumentList "$proj /p:Configuration=RelWithDebInfo /m /nologo /v:minimal" `
    -WorkingDirectory $buildDir `
    -Wait -PassThru `
    -RedirectStandardOutput $outFile `
    -RedirectStandardError $errFile

Get-Content $outFile | Select-Object -Last 5
exit $proc.ExitCode
