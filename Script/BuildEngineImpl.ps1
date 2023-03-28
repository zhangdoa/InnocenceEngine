 param (
    [string]$buildType
 )

 Set-Location ../Source/External/Tools
$msbuildPath = .\vswhere.exe -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1

Set-Location ../../../
Set-Location Build

mkdir $buildType
Set-Location $buildType

$genArgs = @('-DBUILD_GAME=ON ../../Source -G "Visual Studio 17"')
$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall
Invoke-Expression $genCall
Invoke-Expression $genCall

$msbuildArgs= "InnocenceEngine.sln /property:Configuration=" + $buildType + " /m"

Start-Process $msbuildPath -ArgumentList $msbuildArgs -Wait

pause