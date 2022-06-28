 param (
    [string]$buildType
 )

Set-Location ../Source/External/Tools
$msbuildPath = .\vswhere.exe -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1

Set-Location ../GitSubmodules/assimp/build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..."

$genArgs = @('-DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF ../../ -G "Visual Studio 16"')

Write-Output "Build solution..."

$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall
Invoke-Expression $genCall

$msbuildArgs = "Assimp.sln /property:Configuration=" + $buildType + " /m"

Start-Process $msbuildPath -ArgumentList $msbuildArgs -Wait

Write-Output "Copy files..."

xcopy /s/e/y include\assimp\config.h ..\..\..\..\Include\assimp\

xcopy /s/e/y code\$buildType\*.dll ..\..\..\..\DLL\Win\$buildType\
xcopy /s/e/y code\$buildType\*.lib ..\..\..\..\Lib\Win\$buildType\

$libName = 'assimp-vc142'

Switch ($buildType)
{
   {$_ -match 'Debug'} {$libName += '-mtd.lib'}
   {$_ -match 'Release'} {$libName += '-mt.lib'}
}

Remove-Item ..\..\..\..\Lib\Win\$buildType\assimp.lib
Rename-Item ..\..\..\..\Lib\Win\$buildType\$libName assimp.lib