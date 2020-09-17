 param (
    [string]$buildType,[string]$toolchain
 )

 $config
 $vc_version

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$config += 'vc15'}
   {$_ -match 'VS16'} {$config += 'vc16'}
}

$config += 'win64'

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$vc_version += 'vc141'}
   {$_ -match 'VS16'} {$vc_version += 'vc142'}
}

Set-Location ../Source/External/GitSubmodules/PhysX/physx

If ($toolchain -match 'VS16')
{
   Write-Output "Patch PsAllocator.h for header file compatibility..."
   $source_code = Get-Content .\source\foundation\include\PsAllocator.h
   $source_code | ForEach-Object { $_.Replace("#include <typeinfo.h>", "#include <typeinfo>") } | Set-Content .\source\foundation\include\PsAllocator.h
}

Write-Output "Change runtime library to MD/MDd..."
$con = Get-Content .\buildtools\presets\public\$config.xml
$con | ForEach-Object { $_.Replace("`"NV_USE_STATIC_WINCRT`" value=`"True`"", "`"NV_USE_STATIC_WINCRT`" value=`"False`"") } | Set-Content .\buildtools\presets\public\$config.xml

Write-Output "Generate projects with current settings..."
Start-Process generate_projects.bat $config -Wait

Write-Output "Build solution..."

$msbuildArgs= "compiler/$config/PhysXSDK.sln /property:Configuration=$buildType /m"
$msbuildPath

Switch ($toolchain)
{
   {$_ -match 'VS15'} {$msbuildPath = $Env:VS2017INSTALLDIR + "\MSBuild\15.0\Bin\msbuild.exe"}
   {$_ -match 'VS16'} {$msbuildPath = $Env:VS2019INSTALLDIR + "\MSBuild\Current\Bin\msbuild.exe"}
}

Start-Process $msbuildPath -ArgumentList $msbuildArgs -Wait

$buildTypeLowerCase = $buildType.ToLower()
xcopy /s/e/y bin\Win.x86_64.$vc_version.md\$buildTypeLowerCase\*.dll ..\..\..\DLL\Win\$buildType\
xcopy /s/e/y bin\Win.x86_64.$vc_version.md\$buildTypeLowerCase\*.lib ..\..\..\Lib\Win\$buildType\
xcopy /s/e/y bin\Win.x86_64.$vc_version.md\$buildTypeLowerCase\*.pdb ..\..\..\Lib\Win\$buildType\