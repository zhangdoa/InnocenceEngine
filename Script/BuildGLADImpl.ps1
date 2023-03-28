 param (
    [string]$buildType
 )

 Set-Location ../Source/External/Tools
 $msbuildPath = .\vswhere.exe -latest -products Microsoft.VisualStudio.Product.BuildTools -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1
 if ($msbuildPath -eq '') {
    $msbuildPath = .\vswhere.exe -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1
 }

Set-Location ../GitSubmodules/GLAD/build

mkdir $buildType
Set-Location $buildType

Write-Output "Generate projects for "$buildType"..."

$genArgs = @(' ../../cmake -G "Visual Studio 17"')

Write-Output "Build solution..."

$genArgs += ('-DCMAKE_BUILD_TYPE={0}' -f $buildType)
$genCall = ('cmake {0}' -f ($genArgs -Join ' '))
Write-Host $genCall
Invoke-Expression $genCall

$msbuildArgs= "ALL_BUILD.vcxproj /property:Configuration=" + $buildType + " /m"

Start-Process $msbuildPath -ArgumentList $msbuildArgs -NoNewWindow -Wait

Write-Output "Copy files..."

xcopy /s/e/y include\* ..\..\..\..\Include\

xcopy /s/e/y $buildType\*.dll ..\..\..\..\DLL\Win\$buildType\
xcopy /s/e/y $buildType\*.lib ..\..\..\..\Lib\Win\$buildType\
xcopy /s/e/y $buildType\*.pdb ..\..\..\..\Lib\Win\$buildType\

# Check if the previous command exited with an error
if ($LASTEXITCODE -ne 0) {
   Read-Host -Prompt "Press Enter to continue"
   exit 1
}