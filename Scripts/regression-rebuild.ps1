# Full rebuild chain: CMake regen + build + clangd index purge.
# Use after rename / file-add / file-delete touches that change CMake's source GLOB.
param(
    [switch]$SkipShaderCompile,
    [switch]$NoClangdPurge
)
$ErrorActionPreference = 'Stop'

cmake -B Build -S .

if ($SkipShaderCompile) {
    & .\Scripts\BuildWin.ps1 -SkipShaderCompile
} else {
    & .\Scripts\BuildWin.ps1
}

if (-not $NoClangdPurge) {
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '.cache\clangd', 'Build\clangd\.cache'
    Write-Host 'clangd index purged.'
}
