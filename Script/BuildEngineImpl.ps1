 param (
    [string]$buildType,[string]$toolchain
 )

Set-Location ../Build

mkdir $buildType
Set-Location $buildType

$genArgs = @('-DINNO_PLATFORM_WIN=ON -DBUILD_GAME=ON ../../Source')

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$genArgs += '-G "Visual Studio 15 Win64"'}
   {$_ -match 'Clang'} {$genArgs += '-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G "Unix Makefiles"'}
}

$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall 
Invoke-Expression $genCall
Invoke-Expression $genCall

msbuild InnocenceEngine.sln /property:Configuration=$buildType /m

pause