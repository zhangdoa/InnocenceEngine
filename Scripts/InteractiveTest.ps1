# InteractiveTest.ps1 — Automated interactive testing for InnocenceEngine
# Launches Main.exe windowed and sends keystrokes to exercise crash-prone code paths.
# Exit codes: 0 = pass, 1 = crash/GPU error, 2 = timeout
#
# Usage:
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1 -Scenario toggle_pathtracer
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1 -Scenario scene_reload
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1 -Scenario camera_movement
#   powershell.exe -NoProfile -File Scripts/InteractiveTest.ps1 -Scenario reimport -TimeoutSeconds 600
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
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
    public const uint WM_KEYDOWN     = 0x0100;
    public const uint WM_KEYUP       = 0x0101;
    public const uint WM_RBUTTONDOWN = 0x0204;
    public const uint WM_RBUTTONUP   = 0x0205;
    public const uint SWP_NOZORDER   = 0x0004;
    public const uint SWP_NOMOVE     = 0x0002;
}
"@

function Send-Key($hwnd, [int]$vk) {
    [Win32]::PostMessage($hwnd, [Win32]::WM_KEYDOWN, [IntPtr]$vk, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 50
    [Win32]::PostMessage($hwnd, [Win32]::WM_KEYUP, [IntPtr]$vk, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
}

# Virtual key codes for engine keybindings
$VK_W = 0x57; $VK_A = 0x41; $VK_S = 0x53; $VK_D = 0x44
$VK_B = 0x42  # Toggle path tracer
$VK_L = 0x4C  # Load GISponza scene
$VK_R = 0x52  # Load test scene
$VK_N = 0x4E  # Run ray tracing
$VK_E = 0x45  # Add force
$VK_H = 0x48  # Light heatmap
$VK_Y = 0x59  # Re-import all models (writes .innobin texture/mesh files)
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
function Run-TogglePT {
    Write-Host "`n--- Toggle Path Tracer ---"

    Write-Host "  Pressing B (path tracer ON)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PT ON"

    Write-Host "  Pressing B (path tracer OFF)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 3 "PT OFF"

    Write-Host "  Pressing B (path tracer ON again)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PT ON again"

    Write-Host "  Pressing B (path tracer OFF again)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 3 "PT OFF again"
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
        Write-Host "  Moving camera: $name"
        for ($i = 0; $i -lt 5; $i++) {
            Send-Key $hwnd $key
        }
        Wait-AndCheck 1 "Camera $name"
    }

    # Release right mouse button
    Write-Host "  Right mouse button UP (disable camera movement)"
    [Win32]::PostMessage($hwnd, [Win32]::WM_RBUTTONUP, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
}

# --- Scenario: Re-import all models (generates .innobin texture/mesh binary files) ---
function Run-ReImport {
    Write-Host "`n--- Model Re-Import ---"
    Write-Host "  Pressing Y (trigger AssetService::Import for all models)"
    Send-Key $hwnd $VK_Y

    # Import of 5 large models (Sponza x2, ShaderBall, bunny, dragon) takes several minutes.
    # Poll for .innobin files in Data/Generated/Components/ to detect completion.
    $generatedDir = Join-Path $PSScriptRoot "..\Data\Generated\Components"
    $pollInterval = 10
    $maxWait = $TimeoutSeconds - 30  # leave 30s buffer for cleanup
    $waited = 0

    Write-Host "  Polling $generatedDir for .innobin files (max ${maxWait}s)..."
    while ($waited -lt $maxWait) {
        Start-Sleep -Seconds $pollInterval
        $waited += $pollInterval
        if (-not (Test-ProcessAlive)) { exit 1 }

        $innobinCount = (Get-ChildItem -Path $generatedDir -Filter "*.innobin" -ErrorAction SilentlyContinue | Measure-Object).Count
        Write-Host "  [${waited}s] .innobin files found: $innobinCount"
        if ($innobinCount -gt 50) {
            Write-Host "  Import appears complete ($innobinCount files). Done."
            break
        }
    }
}

# --- Scenario: GISponza scene load and camera walkthrough ---
function Run-GISponza {
    Write-Host "`n--- GISponza Scene Load ---"

    Write-Host "  Pressing L (load GISponza scene)"
    Send-Key $hwnd $VK_L
    Wait-AndCheck 20 "GISponza loading"

    Write-Host "`n--- Camera walkthrough in Sponza ---"
    [Win32]::PostMessage($hwnd, [Win32]::WM_RBUTTONDOWN, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200

    foreach ($key in @($VK_W, $VK_W, $VK_W, $VK_A, $VK_D, $VK_S)) {
        $name = @{ $VK_W="W"; $VK_A="A"; $VK_S="S"; $VK_D="D" }[$key]
        Write-Host "  Moving camera: $name"
        for ($i = 0; $i -lt 5; $i++) {
            Send-Key $hwnd $key
        }
        Wait-AndCheck 1 "Camera $name"
    }

    [Win32]::PostMessage($hwnd, [Win32]::WM_RBUTTONUP, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200

    Wait-AndCheck 3 "GISponza stable"
}

# --- Scenario: Path tracer + window resize (TASK-113) ---
# Exercises PTPass::OnResize — the accumulation buffer must be
# recreated at the new resolution and accumulation history scrapped.
function Run-PTResize {
    Write-Host "`n--- Path Tracer + Window Resize ---"

    Write-Host "  Pressing B (path tracer ON)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PT ON"

    $flags = [Win32]::SWP_NOZORDER -bor [Win32]::SWP_NOMOVE

    foreach ($dims in @(@(1024, 768), @(1600, 900), @(800, 600))) {
        $w = $dims[0]; $h = $dims[1]
        Write-Host "  Resize to ${w}x${h}"
        [Win32]::SetWindowPos($hwnd, [IntPtr]::Zero, 0, 0, $w, $h, $flags) | Out-Null
        Wait-AndCheck 3 "Resize ${w}x${h} stable"
    }
}

# --- Scenario: Path tracer + scene reload (the dangerous combo) ---
function Run-PTWithReload {
    Write-Host "`n--- Path Tracer + Scene Reload ---"

    Write-Host "  Pressing B (path tracer ON)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PT ON"

    Write-Host "  Pressing R (reload scene while path tracer active)"
    Send-Key $hwnd $VK_R
    Wait-AndCheck 10 "Scene reload with PT"

    Write-Host "  Pressing B (toggle path tracer after reload)"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PT toggle post-reload"
}

# --- Execute scenario ---
switch ($Scenario) {
    "toggle_pathtracer"  { Run-TogglePT }
    "scene_reload"       { Run-SceneReload }
    "camera_movement"    { Run-CameraMovement }
    "pathtracer_reload"  { Run-PTWithReload }
    "pathtracer_resize"  { Run-PTResize }
    "gi_sponza"          { Run-GISponza }
    "reimport"           { Run-ReImport }
    "full" {
        Run-CameraMovement
        Run-TogglePT
        Run-SceneReload
        Run-PTWithReload
        Run-PTResize
    }
    default {
        Write-Host "Unknown scenario: $Scenario"
        Write-Host "Valid: toggle_pathtracer, scene_reload, camera_movement, pathtracer_reload, pathtracer_resize, gi_sponza, reimport, full"
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
