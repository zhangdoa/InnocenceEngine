Set-Variable -Name "currentDir" -Value (Get-Location).path

mkdir ../build

Start-Process powershell.exe -ArgumentList "./buildEngineImpl.ps1", "-buildType `"Debug`""
Start-Process powershell.exe -ArgumentList "./buildEngineImpl.ps1", "-buildType `"Release`""