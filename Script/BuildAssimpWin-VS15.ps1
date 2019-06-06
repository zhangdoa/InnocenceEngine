mkdir ../Source/External/GitSubmodules/assimp/build

Start-Process powershell.exe -ArgumentList "./BuildAssimpImpl.ps1", "-buildType `"Debug`""
Start-Process powershell.exe -ArgumentList "./BuildAssimpImpl.ps1", "-buildType `"Release`""