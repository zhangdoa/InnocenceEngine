 param (
    [string]$buildType
 )

Set-Location ../build

mkdir $buildType
Set-Location $buildType

$genArgs = @('-DINNO_PLATFORM_WIN=ON -DBUILD_GAME=ON -G "Visual Studio 15 Win64" ../../source')
$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall 
Invoke-Expression $genCall
Invoke-Expression $genCall

msbuild InnocenceEngine.sln /property:Configuration=$buildType /m

pause