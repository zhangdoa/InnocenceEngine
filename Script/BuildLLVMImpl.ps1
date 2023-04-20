 param (
    [string]$buildType
 )

Set-Location ../Source/External/Tools
$msbuildPath = .\vswhere.exe -latest -products Microsoft.VisualStudio.Product.BuildTools -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1
if ($msbuildPath -eq '') {
   $msbuildPath = .\vswhere.exe -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1
}

Set-Location ../GitSubmodules/llvm-project/build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..."

$genArgs = @('-S ../../llvm -B build -G "Visual Studio 17" -DLLVM_ENABLE_PROJECTS="clang"')

Write-Output "Build solution..."

$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall
Invoke-Expression $genCall

$msbuildArgs = "build/LLVM.sln /property:Configuration=" + $buildType + " /m"

Start-Process $msbuildPath -ArgumentList $msbuildArgs -NoNewWindow -Wait

# Write-Output "Copy files..."

# xcopy /s/e/y bin\$buildType\*.dll ..\..\..\..\DLL\Win\$buildType\
# xcopy /s/e/y lib\$buildType\*.lib ..\..\..\..\Lib\Win\$buildType\

# $libName = 'assimp-vc143'

# Switch ($buildType)
# {
#    {$_ -match 'Debug'} {$libName += '-mtd.lib'}
#    {$_ -match 'Release'} {$libName += '-mt.lib'}
# }

# Remove-Item ..\..\..\..\Lib\Win\$buildType\assimp.lib
# Rename-Item ..\..\..\..\Lib\Win\$buildType\$libName assimp.lib

# Check if the previous command exited with an error
if ($LASTEXITCODE -ne 0) {
   Read-Host -Prompt "Press Enter to continue"
   exit 1
}