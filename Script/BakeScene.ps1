 param (
    [string]$sceneName
 )

Write-Output "Start to bake scene..."

Set-Location ../Bin/

Write-Output "Generate probe cache and surfel cache for $sceneName scene..."
Start-Process Debug/InnoBaker.exe -ArgumentList "-renderer 1 -mode 0 -loglevel 0 -bakestage probe Res//Scenes//$sceneName.InnoScene" -Wait

Write-Output "Generate brick cache for $sceneName scene..."
Start-Process Debug/InnoBaker.exe -ArgumentList "-renderer 1 -mode 0 -loglevel 0 -bakestage brickcache Res//Intermediate//$sceneName.InnoSurfelCache" -Wait

Write-Output "Generate brick for $sceneName scene..."
Start-Process Debug/InnoBaker.exe -ArgumentList "-renderer 1 -mode 0 -loglevel 0 -bakestage brick Res//Intermediate//$sceneName.InnoBrickCacheSummary" -Wait

Write-Output "Generate brick factor and probe for $sceneName scene..."
Start-Process Debug/InnoBaker.exe -ArgumentList "-renderer 1 -mode 0 -loglevel 0 -bakestage brickfactor Res//Scenes//$sceneName.InnoBrick probecache Res//Intermediate//$sceneName.InnoProbeCache" -Wait
pause