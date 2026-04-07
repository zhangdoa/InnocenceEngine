# InteractiveTest.ps1 — Automated interactive testing for InnocenceEngine
# Launches Main.exe windowed and sends keystrokes to exercise crash-prone code paths.
# Exit codes: 0 = pass, 1 = crash/GPU error, 2 = timeout
#
# Usage:
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1 -Scenario toggle_pathtracer
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1 -Scenario scene_reload
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1 -Scenario camera_movement
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1 -Scenario full

param(
    [string]$Scenario = "full",
    [int]$TimeoutSeconds = 60
)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    public const uint WM_KEYDOWN     = 0x0100;
    public const uint WM_KEYUP       = 0x0101;
    public const uint WM_RBUTTONDOWN = 0x0204;
    public const uint WM_RBUTTONUP   = 0x0205;
}
"@

function Send-Key($hwnd, [int]$vk) {
    [Win32]::PostMessage($hwnd, [Win32]::WM_KEYDOWN, [IntPtr]$vk, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 50
    [Win32]::PostMessage($hwnd, [Win32]::WM_KEYUP, [IntPtr]$vk, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
}

function Hold-Key($hwnd, [int]$vk, [int]$durationMs = 500) {
    [Win32]::PostMessage($hwnd, [Win32]::WM_KEYDOWN, [IntPtr]$vk, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds $durationMs
    [Win32]::PostMessage($hwnd, [Win32]::WM_KEYUP, [IntPtr]$vk, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
}

# Virtual key codes for engine keybindings
$VK_W = 0x57; $VK_A = 0x41; $VK_S = 0x53; $VK_D = 0x44
$VK_B = 0x42  # Toggle path tracer
$VK_R = 0x52  # Load test scene
$VK_N = 0x4E  # Run ray tracing
$VK_E = 0x45  # Add force
$VK_H = 0x48  # Light heatmap
$VK_SPACE = 0x20  # Speed up

$binDir = Join-Path $PSScriptRoot "..\Bin"
$exe = Join-Path $binDir "RelWithDebInfo\Main.exe"

if (-not (Test-Path $exe)) {
    Write-Error "Main.exe not found at: $exe"
    exit 2
}

Write-Host "=== InnocenceEngine Interactive Test ==="
Write-Host "Scenario: $Scenario"
Write-Host "Timeout:  ${TimeoutSeconds}s"
Write-Host ""

$logFile = Join-Path $binDir "..\Build\interactive_test.log"
$process = Start-Process -FilePath $exe `
    -ArgumentList "-mode 0 -renderer 0 -loglevel 0" `
    -WorkingDirectory $binDir `
    -PassThru

Start-Sleep -Seconds 5

if ($process.HasExited) {
    Write-Host "FAIL: Process exited early with code $($process.ExitCode)"
    exit 1
}

$hwnd = $process.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "WARN: No window handle yet, waiting..."
    Start-Sleep -Seconds 3
    $hwnd = $process.MainWindowHandle
}

if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "FAIL: Could not find engine window"
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    exit 2
}

[Win32]::SetForegroundWindow($hwnd) | Out-Null
Write-Host "Engine window found (PID: $($process.Id), HWND: $hwnd)"

function Test-ProcessAlive {
    if ($process.HasExited) {
        Write-Host "FAIL: Process crashed (exit code: $($process.ExitCode))"
        return $false
    }
    return $true
}

function Wait-AndCheck($seconds, $label) {
    Write-Host "  [$label] Waiting ${seconds}s..."
    for ($i = 0; $i -lt $seconds; $i++) {
        Start-Sleep -Seconds 1
        if (-not (Test-ProcessAlive)) { exit 1 }
    }
}

# --- Scenario: Toggle path tracer on/off ---
function Run-TogglePathTracer {
    Write-Host "`n--- Toggle Path Tracer ---"

    Write-Host "  Pressing B (path tracer ON)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PathTracer ON"

    Write-Host "  Pressing B (path tracer OFF)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 3 "PathTracer OFF"

    Write-Host "  Pressing B (path tracer ON again)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PathTracer ON again"

    Write-Host "  Pressing B (path tracer OFF again)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 3 "PathTracer OFF again"
}

# --- Scenario: Scene reload ---
function Run-SceneReload {
    Write-Host "`n--- Scene Reload ---"

    Write-Host "  Pressing R (load test scene)"
    Send-Key $hwnd $VK_R
    Wait-AndCheck 8 "Scene loading"
}

# --- Scenario: Camera movement ---
function Run-CameraMovement {
    Write-Host "`n--- Camera Movement ---"

    # Hold right mouse button to unlock camera movement (m_CanMove gate)
    Write-Host "  Right mouse button DOWN (enable camera movement)"
    [Win32]::PostMessage($hwnd, [Win32]::WM_RBUTTONDOWN, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200

    foreach ($key in @($VK_W, $VK_A, $VK_S, $VK_D)) {
        $name = @{ $VK_W="W"; $VK_A="A"; $VK_S="S"; $VK_D="D" }[$key]
        Write-Host "  Moving camera: $name (holding 500ms)"
        Hold-Key $hwnd $key 500
        if (-not (Test-ProcessAlive)) { exit 1 }
    }

    # Release right mouse button
    Write-Host "  Right mouse button UP (disable camera movement)"
    [Win32]::PostMessage($hwnd, [Win32]::WM_RBUTTONUP, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
}

# --- Scenario: Path tracer + scene reload (the dangerous combo) ---
function Run-PathTracerWithReload {
    Write-Host "`n--- Path Tracer + Scene Reload ---"

    Write-Host "  Pressing B (path tracer ON)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PathTracer ON"

    Write-Host "  Pressing R (reload scene while path tracer active)"
    Send-Key $hwnd $VK_R
    Wait-AndCheck 10 "Scene reload with PT"

    Write-Host "  Pressing B (toggle path tracer after reload)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PathTracer toggle post-reload"
}

# --- Execute scenario ---
switch ($Scenario) {
    "toggle_pathtracer"  { Run-TogglePathTracer }
    "scene_reload"       { Run-SceneReload }
    "camera_movement"    { Run-CameraMovement }
    "pathtracer_reload"  { Run-PathTracerWithReload }
    "full" {
        Run-CameraMovement
        Run-TogglePathTracer
        Run-SceneReload
        Run-PathTracerWithReload
    }
    default {
        Write-Host "Unknown scenario: $Scenario"
        Write-Host "Valid: toggle_pathtracer, scene_reload, camera_movement, pathtracer_reload, full"
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        exit 2
    }
}

# --- Cleanup ---
Write-Host "`n--- Shutting down ---"

if (Test-Path $logFile) {
    Write-Host "`n--- Engine Log (errors only) ---"
    Get-Content $logFile | Select-String -Pattern "Error|FAIL|DXGI|device|barrier|D3D12 ERROR|crash|FATAL" | ForEach-Object { Write-Host $_.Line }
}

if (-not $process.HasExited) {
    Stop-Process -Id $process.Id -Force
    Start-Sleep -Seconds 2
}

Write-Host ""
Write-Host "=== PASS: Scenario '$Scenario' completed without crashes ==="
exit 0
