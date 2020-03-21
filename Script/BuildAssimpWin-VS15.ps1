mkdir ../Source/External/GitSubmodules/assimp/build

Start-Process powershell.exe -ArgumentList "./BuildAssimpImpl.ps1", "-buildType `"Debug`"", "-toolchain `"VS15`""
Start-Process powershell.exe -ArgumentList "./BuildAssimpImpl.ps1", "-buildType `"Release`"", "-toolchain `"VS15`""