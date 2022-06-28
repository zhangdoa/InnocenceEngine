 param (
    [string]$buildType
 )

Set-Location ../Source/External/Tools
$msbuildPath = .\vswhere.exe -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | select-object -first 1

Set-Location ../GitSubmodules/PhysX/physx

$config = 'vc16'
$vc_version = 'vc142'

$config += 'win64'

Write-Output "Patch PsAllocator.h for header file compatibility..."
$source_code = Get-Content .\source\foundation\include\PsAllocator.h
$source_code | ForEach-Object { $_.Replace("#include <typeinfo.h>", "#include <typeinfo>") } | Set-Content .\source\foundation\include\PsAllocator.h

Write-Output "Change runtime library to MD/MDd..."
$con = Get-Content .\buildtools\presets\public\$config.xml
$con | ForEach-Object { $_.Replace("`"NV_USE_STATIC_WINCRT`" value=`"True`"", "`"NV_USE_STATIC_WINCRT`" value=`"False`"") } | Set-Content .\buildtools\presets\public\$config.xml

Write-Output "Disable building samples..."
$con | ForEach-Object { $_.Replace("`"PX_BUILDPUBLICSAMPLES`" value=`"True`"", "`"PX_BUILDPUBLICSAMPLES`" value=`"False`"") } | Set-Content .\buildtools\presets\public\$config.xml

Write-Output "Generate projects with current settings..."
Start-Process generate_projects.bat $config -Wait

Write-Output "Build solution..."

$msbuildArgs= "compiler/$config/PhysXSDK.sln /property:Configuration=$buildType /m"

Start-Process $msbuildPath -ArgumentList $msbuildArgs -Wait

$buildTypeLowerCase = $buildType.ToLower()
xcopy /s/e/y bin\Win.x86_64.$vc_version.md\$buildTypeLowerCase\*.dll ..\..\..\DLL\Win\$buildType\
xcopy /s/e/y bin\Win.x86_64.$vc_version.md\$buildTypeLowerCase\*.lib ..\..\..\Lib\Win\$buildType\
xcopy /s/e/y bin\Win.x86_64.$vc_version.md\$buildTypeLowerCase\*.pdb ..\..\..\Lib\Win\$buildType\