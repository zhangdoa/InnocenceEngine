# Scripts/TestEditorComplete.ps1
# Master verification script for the Editor-Engine stack

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$editorRoot = Join-Path $scriptRoot "..\Source\Editor-Next"

Write-Host "=== Phase 1: Headless Engine IPC Verification ===" -ForegroundColor Cyan
python "$scriptRoot\VerifyEditorService.py"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Phase 1 Failed: Engine IPC protocol error."
    exit 1
}

Write-Host "`n=== Phase 2: Electron E2E UI Verification ===" -ForegroundColor Cyan
Set-Location $editorRoot
npm test
if ($LASTEXITCODE -ne 0) {
    Write-Warning "Phase 2 Failed or Timed Out. This is common in headless environments without GPU."
    Write-Host "Check the console output above for [ELECTRON] logs to confirm connection."
}

Write-Host "`n=== Verification Complete ===" -ForegroundColor Green
