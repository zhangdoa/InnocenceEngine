# Scripts/VerifyEditorStack.ps1
# End-to-end verification of the Editor-Engine stack

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$editorRoot = Join-Path $scriptRoot "..\Source\Editor-Next"
$binDir = Join-Path $scriptRoot "..\Bin"
$engineExe = Join-Path $binDir "RelWithDebInfo\Main.exe"

Write-Host "=== Starting E2E Stack Verification ===" -ForegroundColor Cyan

# 1. Kill any existing instances
taskkill /F /IM Main.exe /T 2>$null
taskkill /F /IM electron.exe /T 2>$null

# 2. Start Engine in a separate job
Write-Host "Launching Engine..." -ForegroundColor Yellow
$engineJob = Start-Job -ScriptBlock {
    param($exe, $cwd)
    Set-Location $cwd
    & $exe -mode 2 -renderer 0 -loglevel 0
} -ArgumentList $engineExe, $binDir

# 3. Start monitoring engine output
$engineStarted = $false
$timeout = 60
$elapsed = 0

while ($elapsed -lt $timeout -and -not $engineStarted) {
    $results = Receive-Job -Job $engineJob
    foreach ($line in $results) {
        Write-Host "ENGINE: $line" -ForegroundColor Gray
        if ($line -like "*WebSocket server started on port 8081*") {
            $engineStarted = $true
        }
    }
    if (-not $engineStarted) {
        Start-Sleep -Seconds 1
        $elapsed++
        # If the job is already failed
        if ($engineJob.State -eq "Failed") {
            Write-Error "Engine job failed."
            break
        }
    }
}

# If it didn't start, show all remaining output
$results = Receive-Job -Job $engineJob
foreach ($line in $results) {
    Write-Host "ENGINE (FINAL): $line" -ForegroundColor Gray
}

if (-not $engineStarted) {
    Write-Error "Engine failed to start WebSocket server within $timeout seconds."
    Stop-Job $engineJob
    exit 1
}

Write-Host "Engine WebSocket server is UP." -ForegroundColor Green

# 4. Launch Electron
Write-Host "Launching Electron Editor..." -ForegroundColor Yellow
Set-Location $editorRoot

# Use a process to capture electron output
$electronProc = Start-Process -FilePath "npm.cmd" -ArgumentList "start" -NoNewWindow -PassThru -RedirectStandardOutput "$scriptRoot\electron_out.txt" -RedirectStandardError "$scriptRoot\electron_err.txt"

# 5. Monitor Electron output for connection success
$connected = $false
$elapsed = 0
while ($elapsed -lt 30 -and -not $connected) {
    if (Test-Path "$scriptRoot\electron_out.txt") {
        $out = Get-Content "$scriptRoot\electron_out.txt" -Tail 10
        foreach ($line in $out) {
            Write-Host "EDITOR: $line" -ForegroundColor Magenta
            if ($line -like "*Main: Connected to Engine*") {
                $connected = $true
            }
        }
    }
    if (-not $connected) {
        Start-Sleep -Seconds 1
        $elapsed++
    }
}

# 6. Final Report
if ($connected) {
    Write-Host "=== SUCCESS: Stack is connected and healthy ===" -ForegroundColor Green
} else {
    Write-Host "=== FAILURE: Editor failed to connect to Engine ===" -ForegroundColor Red
    if (Test-Path "$scriptRoot\electron_err.txt") {
        Write-Host "--- Editor Errors ---" -ForegroundColor Red
        Get-Content "$scriptRoot\electron_err.txt"
    }
}

# Cleanup
Stop-Job $engineJob
Stop-Process -Id $electronProc.Id -Force -ErrorAction SilentlyContinue
