 param (
    [string]$buildType
 )

Set-Location ../source/external/gitsubmodules/PhysX/physx
msbuild compiler/vc15win64/PhysXSDK.sln /property:Configuration=$buildType /m

$buildTypeLowerCase = $buildType.ToLower()
xcopy /s/e/y bin\win.x86_64.vc141.md\$buildTypeLowerCase\*.dll ..\..\..\dll\win\$buildType\
xcopy /s/e/y bin\win.x86_64.vc141.md\$buildTypeLowerCase\*.lib ..\..\..\lib\win\$buildType\