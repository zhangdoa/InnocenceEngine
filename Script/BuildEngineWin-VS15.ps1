mkdir ../Build

Start-Process powershell.exe -ArgumentList "./BuildEngineImpl.ps1", "-buildType `"Debug`"", "-toolchain `"VS15`""
#Start-Process powershell.exe -ArgumentList "./BuildEngineImpl.ps1", "-buildType `"Release`"", "-toolchain `"VS15`""