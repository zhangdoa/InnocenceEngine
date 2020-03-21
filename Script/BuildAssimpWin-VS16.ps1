mkdir ../Source/External/GitSubmodules/assimp/build

Start-Process powershell.exe -ArgumentList "./BuildAssimpImpl.ps1", "-buildType `"Debug`"", "-toolchain `"VS16`""
Start-Process powershell.exe -ArgumentList "./BuildAssimpImpl.ps1", "-buildType `"Release`"", "-toolchain `"VS16`""