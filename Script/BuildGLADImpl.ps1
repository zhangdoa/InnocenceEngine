 param (
    [string]$buildType
 )

 Set-Location ../Source/External/Tools
$msbuildPath = .\vswhere.exe -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1

Set-Location ../GitSubmodules/GLAD/build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..."

$genArgs = @(' ../../ -G "Visual Studio 16"')

Write-Output "Build solution..."

$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall
Invoke-Expression $genCall

$msbuildArgs= "GLAD.sln /property:Configuration=" + $buildType + " /m"

Start-Process $msbuildPath -ArgumentList $msbuildArgs -Wait

Write-Output "Copy files..."

xcopy /s/e/y include\* ..\..\..\..\Include\

xcopy /s/e/y $buildType\*.dll ..\..\..\..\DLL\Win\$buildType\
xcopy /s/e/y $buildType\*.lib ..\..\..\..\Lib\Win\$buildType\
xcopy /s/e/y $buildType\*.pdb ..\..\..\..\Lib\Win\$buildType\