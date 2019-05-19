mkdir ../source/external/gitsubmodules/assimp/build

Start-Process powershell.exe -ArgumentList "./buildAssimpImpl.ps1", "-buildType `"Debug`""
Start-Process powershell.exe -ArgumentList "./buildAssimpImpl.ps1", "-buildType `"Release`""