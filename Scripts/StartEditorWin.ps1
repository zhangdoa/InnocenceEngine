# Scripts/StartEditorWin.ps1
# Automates the setup and launch of the InnocenceEngine Sidecar Editor

param (
    [Parameter(Mandatory=$false)]
    [ValidateSet("RenderTest", "Main")]
    [string]$EngineType = "Main",

    [Parameter(Mandatory=$false)]
    [switch]$BuildOnly
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$editorRoot = Join-Path $scriptRoot "..\Source\Editor-Next"
$binDir = Join-Path $scriptRoot "..\Bin\RelWithDebInfo"

# 1. Verify Engine Binaries
$engineExe = Join-Path $binDir "$EngineType.exe"
if (-not (Test-Path $engineExe)) {
    Write-Error "Engine binary not found at $engineExe. Please build the engine first."
    exit 1
}

Write-Host "--- InnocenceEngine Editor Setup ---" -ForegroundColor Cyan

# 2. Check for Node.js
if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
    Write-Error "npm not found. Please install Node.js to use the Editor."
    exit 1
}

Set-Location $editorRoot

# 3. Install Dependencies if node_modules is missing
if (-not (Test-Path "node_modules")) {
    Write-Host "Installing frontend dependencies..." -ForegroundColor Yellow
    npm install
}

# 4. Build the Vue frontend
Write-Host "Building Vue frontend..." -ForegroundColor Yellow
npm run build

if ($BuildOnly) {
    Write-Host "Build complete." -ForegroundColor Green
    exit 0
}

# 5. Launch Editor
Write-Host "Launching Editor with $EngineType sidecar..." -ForegroundColor Green
npm start -- --engine=$EngineType
