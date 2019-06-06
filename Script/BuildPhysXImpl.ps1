 param (
    [string]$buildType
 )

Set-Location ../Source/External/GitSubmodules/PhysX/physx
msbuild compiler/vc15win64/PhysXSDK.sln /property:Configuration=$buildType /m

$buildTypeLowerCase = $buildType.ToLower()
xcopy /s/e/y bin\Win.x86_64.vc141.md\$buildTypeLowerCase\*.dll ..\..\..\DLL\Win\$buildType\
xcopy /s/e/y bin\Win.x86_64.vc141.md\$buildTypeLowerCase\*.lib ..\..\..\Lib\Win\$buildType\
xcopy /s/e/y bin\Win.x86_64.vc141.md\$buildTypeLowerCase\*.pdb ..\..\..\Lib\Win\$buildType\