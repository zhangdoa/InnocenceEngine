mkdir ../Build

Start-Process powershell.exe -ArgumentList "./BuildEngineImpl.ps1", "-buildType `"Debug`"", "-toolchain `"VS16`""
#Start-Process powershell.exe -ArgumentList "./BuildEngineImpl.ps1", "-buildType `"Release`"", "-toolchain `"VS16`""