mkdir ..\Res\Shaders\DXIL
Set-Location ../Res/Shaders/DXIL
Remove-Item *.dxil

Set-Location ../HLSL

$files = Get-ChildItem
foreach ($file in $files) {
    $name=$file.Name
    $extension=$file.Extension
    $target_profile="lib_6_0"
    if ($extension -match ".vert") {
        $target_profile="vs_6_0"
    } elseif ($extension -match ".tesc") {
        $target_profile="hs_6_0"
    } elseif ($extension -match ".tese") {
        $target_profile="ds_6_0"
    } elseif ($extension -match ".geom") {
        $target_profile="gs_6_0"
    } elseif ($extension -match ".frag") {
        $target_profile="ps_6_0"
    } elseif ($extension -match ".comp") {
        $target_profile="cs_6_0"
    }
    
    $genCall = "dxc -Wno-ignored-attributes -Qembed_debug -T $target_profile -E main -Fo ../DXIL/$name.dxil $file /Zi /Zss"
    Invoke-Expression $genCall
}
pause