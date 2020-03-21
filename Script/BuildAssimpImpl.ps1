 param (
    [string]$buildType,[string]$toolchain
 )

Set-Location ../Source/External/GitSubmodules/assimp/build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..."

$genArgs = @('-DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF ../../')

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$genArgs += '-G "Visual Studio 15 Win64"'}
   {$_ -match 'VS16'} {$genArgs += '-G "Visual Studio 16"'}
}

Write-Output "Build solution..."

$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall
Invoke-Expression $genCall

$msbuildArgs= "Assimp.sln /property:Configuration=" + $buildType + " /m"
$msbuildPath

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$msbuildPath = $Env:VS2017INSTALLDIR + "\MSBuild\15.0\Bin\msbuild.exe"}
   {$_ -match 'VS16'} {$msbuildPath = $Env:VS2019INSTALLDIR + "\MSBuild\Current\Bin\msbuild.exe"}
}

Start-Process $msbuildPath -ArgumentList $msbuildArgs -Wait

Write-Output "Copy files..."

xcopy /s/e/y include\assimp\config.h ..\..\..\..\Include\assimp\

xcopy /s/e/y code\$buildType\*.dll ..\..\..\..\DLL\Win\$buildType\
xcopy /s/e/y code\$buildType\*.lib ..\..\..\..\Lib\Win\$buildType\

$libName

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$libName = 'assimp-vc141'}
   {$_ -match 'VS16'} {$libName = 'assimp-vc142'}
}

Switch ($buildType)
{
   {$_ -match 'Debug'} {$libName += '-mtd.lib'}
   {$_ -match 'Release'} {$libName += '-mt.lib'}
}

Remove-Item ..\..\..\..\Lib\Win\$buildType\assimp.lib
Rename-Item ..\..\..\..\Lib\Win\$buildType\$libName assimp.lib