 param (
    [string]$buildType
 )

Set-Location ../Source/External/GitSubmodules/assimp/build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..."

cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Visual Studio 15 Win64" ../../

Write-Output "Build solution..."

msbuild Assimp.sln /property:Configuration=$buildType /m

Write-Output "Copy files..."

xcopy /s/e/y include\assimp\config.h ..\..\..\..\Include\assimp\

xcopy /s/e/y code\$buildType\*.dll ..\..\..\..\DLL\Win\$buildType\
xcopy /s/e/y code\$buildType\*.lib ..\..\..\..\Lib\Win\$buildType\

$libName = 'assimp-vc141'

Switch ($buildType)
{
   {$_ -match 'Debug'} {$libName += '-mtd.lib'}
   {$_ -match 'Release'} {$libName += '-mt.lib'}
}

Remove-Item ..\..\..\..\Lib\Win\$buildType\assimp.lib
Rename-Item ..\..\..\..\Lib\Win\$buildType\$libName assimp.lib