Set-Variable -Name scriptDir -Value (Resolve-Path .\).Path

Set-Location ../source/external/gitsubmodules/PhysX/physx
Write-Output "Generate projects with default settings..."
Start-Process generate_projects.bat vc15win64 -Wait

Write-Output "Change runtime library to MD/MDd..."
$con = Get-Content .\buildtools\presets\public\vc15win64.xml
$con | % { $_.Replace("`"NV_USE_STATIC_WINCRT`" value=`"True`"", "`"NV_USE_STATIC_WINCRT`" value=`"False`"") } | Set-Content .\buildtools\presets\public\vc15win64.xml

Write-Output "Generate projects with current settings..."
Start-Process generate_projects.bat vc15win64 -Wait

Set-Location $scriptDir

Write-Output "Build solution..."

Start-Process powershell.exe -ArgumentList "./buildPhysXImpl.ps1", "-buildType `"Debug`""
Start-Process powershell.exe -ArgumentList "./buildPhysXImpl.ps1", "-buildType `"Release`""