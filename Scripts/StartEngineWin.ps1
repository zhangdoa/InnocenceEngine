$binDir = Join-Path $PSScriptRoot '..\Bin'
$proc = Start-Process -FilePath (Join-Path $binDir 'RelWithDebInfo\Main.exe') `
    -ArgumentList '-mode 0 -renderer 0 -loglevel 0' `
    -WorkingDirectory $binDir `
    -Wait -PassThru -NoNewWindow
exit $proc.ExitCode
