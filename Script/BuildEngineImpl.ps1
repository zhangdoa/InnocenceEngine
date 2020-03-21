 param (
    [string]$buildType,[string]$toolchain
 )

Set-Location ../Build

mkdir $buildType
Set-Location $buildType

$genArgs = @('-DBUILD_GAME=ON ../../Source')

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$genArgs += '-G "Visual Studio 15 Win64"'}
   {$_ -match 'VS16'} {$genArgs += '-G "Visual Studio 16"'}
}

$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall
Invoke-Expression $genCall
Invoke-Expression $genCall

$msbuildArgs= "InnocenceEngine.sln /property:Configuration=" + $buildType + " /m"
$msbuildPath

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$msbuildPath = $Env:VS2017INSTALLDIR + "\MSBuild\15.0\Bin\msbuild.exe"}
   {$_ -match 'VS16'} {$msbuildPath = $Env:VS2019INSTALLDIR + "\MSBuild\Current\Bin\msbuild.exe"}
}

Start-Process $msbuildPath -ArgumentList $msbuildArgs -Wait


pause