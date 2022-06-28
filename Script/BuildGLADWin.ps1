mkdir ../Source/External/GitSubmodules/GLAD/Build

Start-Process powershell.exe -ArgumentList "./BuildGLADImpl.ps1", "-buildType `"Debug`""
Start-Process powershell.exe -ArgumentList "./BuildGLADImpl.ps1", "-buildType `"Release`""