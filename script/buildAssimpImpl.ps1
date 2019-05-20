 param (
    [string]$buildType
 )

function Green
{
    process { Write-Host $_ -ForegroundColor Green }
}

Set-Location ../source/external/gitsubmodules/assimp/build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..." | Green

cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Visual Studio 15 Win64" ../../

Write-Output "Build solution..." | Green

msbuild Assimp.sln /property:Configuration=$buildType /m

Write-Output "Copy files..." | Green

xcopy /s/e/y include\assimp\config.h ..\..\..\..\include\assimp\

xcopy /s/e/y code\$buildType\*.dll ..\..\..\..\dll\win\$buildType\
xcopy /s/e/y code\$buildType\*.lib ..\..\..\..\lib\win\$buildType\
Rename-Item ..\..\..\..\lib\win\$buildType\assimp-vc140-mt.lib assimp.lib