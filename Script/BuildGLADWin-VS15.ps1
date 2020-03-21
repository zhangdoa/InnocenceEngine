mkdir ../Source/External/GitSubmodules/GLAD/Build

Start-Process powershell.exe -ArgumentList "./BuildGLADImpl.ps1", "-buildType `"Debug`"", "-toolchain `"VS15`""
Start-Process powershell.exe -ArgumentList "./BuildGLADImpl.ps1", "-buildType `"Release`"", "-toolchain `"VS15`""