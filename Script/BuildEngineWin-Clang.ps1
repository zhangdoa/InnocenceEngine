mkdir ../Build

Start-Process powershell.exe -ArgumentList "./BuildEngineImpl.ps1", "-buildType `"Debug`"", "-toolchain `"Clang`""
#Start-Process powershell.exe -ArgumentList "./BuildEngineImpl.ps1", "-buildType `"Release`"", "-toolchain `"Clang`""