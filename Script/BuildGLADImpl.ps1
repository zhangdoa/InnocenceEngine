 param (
    [string]$buildType,[string]$toolchain
 )

Set-Location ../Source/External/GitSubmodules/GLAD/Build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..."

$genArgs = @(' ../../')

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

$msbuildArgs= "GLAD.sln /property:Configuration=" + $buildType + " /m"
$msbuildPath

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$msbuildPath = $Env:VS2017INSTALLDIR + "\MSBuild\15.0\Bin\msbuild.exe"}
   {$_ -match 'VS16'} {$msbuildPath = $Env:VS2019INSTALLDIR + "\MSBuild\Current\Bin\msbuild.exe"}
}

Start-Process $msbuildPath -ArgumentList $msbuildArgs -Wait

Write-Output "Copy files..."

xcopy /s/e/y include\* ..\..\..\..\Include\

xcopy /s/e/y $buildType\*.dll ..\..\..\..\DLL\Win\$buildType\
xcopy /s/e/y $buildType\*.lib ..\..\..\..\Lib\Win\$buildType\
xcopy /s/e/y $buildType\*.pdb ..\..\..\..\Lib\Win\$buildType\