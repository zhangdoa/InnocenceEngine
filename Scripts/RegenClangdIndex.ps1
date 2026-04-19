# Regenerate compile_commands.json so clangd can resolve project includes.
#
# The Visual Studio CMake generator that drives the engine build does not
# emit compile_commands.json. This script configures a separate Ninja
# build dir purely to produce the database (no compilation happens),
# then copies it to the repo root where clangd auto-discovers it.
#
# Re-run after:
#  - Adding a new .cpp / .h file
#  - Changing CMakeLists.txt (include paths, definitions, c++ standard)
#  - Pulling submodule updates that change external include layout
#
# Requires Ninja from the latest Visual Studio install (located via
# vswhere) plus the MSVC toolchain set up by VsDevCmd.bat.

$ErrorActionPreference = 'Stop'

$repoRoot   = Resolve-Path (Join-Path $PSScriptRoot '..')
$indexDir   = Join-Path $repoRoot 'Build/clangd'
$dbInIndex  = Join-Path $indexDir 'compile_commands.json'
$dbAtRoot   = Join-Path $repoRoot 'compile_commands.json'

# Locate the latest Visual Studio install via vswhere. Both Ninja and the
# MSVC toolchain (cl.exe, link.exe, include / lib paths) live under it; the
# Ninja CMake generator needs all of them at configure time, not just ninja
# itself, because it probes the compiler with a tiny test build.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found at $vswhere. Is Visual Studio installed?"
    exit 1
}
$vsInstall = & $vswhere -latest -property installationPath
if (-not $vsInstall) {
    Write-Error "vswhere returned no Visual Studio installation."
    exit 1
}
$vsDevCmd = Join-Path $vsInstall 'Common7/Tools/VsDevCmd.bat'
if (-not (Test-Path $vsDevCmd)) {
    Write-Error "VsDevCmd.bat not found at $vsDevCmd."
    exit 1
}
Write-Host "Using Visual Studio install: $vsInstall"

New-Item -ItemType Directory -Force -Path $indexDir | Out-Null

# VsDevCmd.bat sets up the entire MSVC environment (cl.exe, ninja, INCLUDE,
# LIB, PATH, etc.) for one cmd.exe session. Chain the cmake configure into
# the same session so it inherits that environment. cmd.exe's exit code
# propagates back via $LASTEXITCODE.
$repoRootStr = $repoRoot.Path
$indexDirStr = $indexDir
$cmdLine = "`"$vsDevCmd`" -arch=x64 -no_logo && cd /d `"$indexDirStr`" && cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON `"$repoRootStr`""
& cmd.exe /c $cmdLine
if ($LASTEXITCODE -ne 0) {
    Write-Error "cmake configure failed (exit $LASTEXITCODE)"
    exit $LASTEXITCODE
}

if (-not (Test-Path $dbInIndex)) {
    Write-Error "Expected compile_commands.json at $dbInIndex but it was not produced."
    exit 1
}

# Copy to repo root so clangd's upward search picks it up from any source
# file. Junction / symlink would avoid the duplication but needs admin on
# Windows; a plain copy is robust and the file is gitignored anyway.
Copy-Item -Path $dbInIndex -Destination $dbAtRoot -Force

Write-Host "Wrote $dbAtRoot ($(((Get-Item $dbAtRoot).Length / 1KB).ToString('N0')) KB)"
Write-Host "clangd will pick this up on its next reload."
