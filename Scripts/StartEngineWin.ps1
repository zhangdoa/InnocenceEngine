param(
    [string]$Preset = 'Engine/Configuration/Presets/Default.json'
)
$binDir = Join-Path $PSScriptRoot '..\Bin'
$proc = Start-Process -FilePath (Join-Path $binDir 'RelWithDebInfo\Main.exe') `
    -ArgumentList "-c $Preset" `
    -WorkingDirectory $binDir `
    -Wait -PassThru -NoNewWindow
exit $proc.ExitCode
