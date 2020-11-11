mkdir ..\Res\Shaders\DXIL
Set-Location ../Res/Shaders/DXIL
Remove-Item *.dxil

Set-Location ../HLSL

$files = Get-ChildItem
foreach ($file in $files) {
    $name=$file.Name
    $extension=$file.Extension
    $target_profile="lib_5_0"
    if ($extension -match ".vert") {
        $target_profile="vs_5_0"
    } elseif ($extension -match ".tesc") {
        $target_profile="hs_5_0"
    } elseif ($extension -match ".tese") {
        $target_profile="ds_5_0"
    } elseif ($extension -match ".geom") {
        $target_profile="gs_5_0"
    } elseif ($extension -match ".frag") {
        $target_profile="ps_5_0"
    } elseif ($extension -match ".comp") {
        $target_profile="cs_5_0"
    }
    
    $genCall = "dxc -Wno-ignored-attributes -T $target_profile -E main -Fo ../DXIL/$name.dxil $file"
    Invoke-Expression $genCall
}
pause