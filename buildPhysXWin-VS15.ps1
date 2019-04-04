function Green
{
    process { Write-Host $_ -ForegroundColor Green }
}

Set-Location source/external/gitsubmodules/PhysX/physx

Write-Output "Generate projects with default settings..." | Green

Start-Process generate_projects.bat vc15win64 -Wait

Write-Output "Change runtime library to MDd..." | Green

$con = Get-Content .\buildtools\presets\public\vc15win64.xml
$con | % { $_.Replace("`"NV_USE_STATIC_WINCRT`" value=`"True`"", "`"NV_USE_STATIC_WINCRT`" value=`"False`"") } | Set-Content .\buildtools\presets\public\vc15win64.xml

Write-Output "Generate projects with current settings..." | Green

Start-Process generate_projects.bat vc15win64 -Wait

Write-Output "Start to build..." | Green

msbuild compiler/vc15win64/PhysXSDK.sln

Write-Output "Copy Dlls and libs..." | Green

xcopy /s/e/y bin\win.x86_64.vc141.md\debug\*.dll ..\..\..\dll\win
xcopy /s/e/y bin\win.x86_64.vc141.md\debug\*.lib ..\..\..\lib\win

pause