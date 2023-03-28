 param (
    [string]$buildType
 )

 Set-Location ../Source/External/Tools
 $msbuildPath = .\vswhere.exe -latest -products Microsoft.VisualStudio.Product.BuildTools -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1
 if ($msbuildPath -eq '') {
    $msbuildPath = .\vswhere.exe -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1
 }

Set-Location ../GitSubmodules/PhysX/physx

$config = 'vc17'
$vc_version = 'vc143'

$config += 'win64'

# Write-Output "Patch PsAllocator.h for header file compatibility..."
# $source_code = Get-Content .\source\foundation\include\PsAllocator.h
# $source_code | ForEach-Object { $_.Replace("#include <typeinfo.h>", "#include <typeinfo>") } | Set-Content .\source\foundation\include\PsAllocator.h

Write-Output "Change runtime library to MD/MDd..."
$con = Get-Content .\buildtools\presets\public\$config.xml
$con | ForEach-Object { $_.Replace("`"NV_USE_STATIC_WINCRT`" value=`"True`"", "`"NV_USE_STATIC_WINCRT`" value=`"False`"") } | Set-Content .\buildtools\presets\public\$config.xml

Write-Output "Generate projects with current settings..."
Start-Process .\generate_projects.bat $config -NoNewWindow -Wait

Write-Output "Build solution..."

$msbuildArgs= "compiler/$config/PhysXSDK.sln /property:Configuration=$buildType /m"

Start-Process $msbuildPath -ArgumentList $msbuildArgs -NoNewWindow -Wait

$buildTypeLowerCase = $buildType.ToLower()
$outputPath = 'win.x86_64.'
$outputPath += $vc_version
$outputPath += '.md'

xcopy /s/e/y bin\$outputPath\$buildTypeLowerCase\*.dll ..\..\..\DLL\Win\$buildType\
xcopy /s/e/y bin\$outputPath\$buildTypeLowerCase\*.lib ..\..\..\Lib\Win\$buildType\

# Check if the previous command exited with an error
if ($LASTEXITCODE -ne 0) {
   Read-Host -Prompt "Press Enter to continue"
   exit 1
}