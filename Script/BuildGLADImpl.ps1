 param (
    [string]$buildType
 )

Set-Location ../Source/External/GitSubmodules/GLAD/Build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..."

cmake -G "Visual Studio 15 Win64" ../../

Write-Output "Build solution..."

msbuild GLAD.sln /property:Configuration=$buildType /m

Write-Output "Copy files..."

xcopy /s/e/y include\* ..\..\..\..\Include\

xcopy /s/e/y $buildType\*.dll ..\..\..\..\DLL\Win\$buildType\
xcopy /s/e/y $buildType\*.lib ..\..\..\..\Lib\Win\$buildType\
xcopy /s/e/y $buildType\*.pdb ..\..\..\..\Lib\Win\$buildType\