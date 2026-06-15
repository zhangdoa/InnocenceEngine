$binDir = Join-Path $PSScriptRoot '..\Bin'
$proc = Start-Process -FilePath (Join-Path $binDir 'RelWithDebInfo\Main.exe') `
    -ArgumentList '-c Data/Engine/Configuration/Presets/Default.json' `
    -WorkingDirectory $binDir `
    -Wait -PassThru -NoNewWindow
exit $proc.ExitCode
