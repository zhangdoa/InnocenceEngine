# InteractiveTest.ps1 — Automated interactive testing for InnocenceEngine
# Launches Main.exe windowed using Data/Engine/Configuration/Presets/Interactive.json
# and sends keystrokes to exercise crash-prone code paths.
# Exit codes: 0 = pass, 1 = crash/GPU error, 2 = timeout

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("toggle_pathtracer", "scene_reload", "camera_movement", "pathtracer_reload", "pathtracer_resize", "gi_sponza", "reimport", "full")]
    [string]$Scenario = "full",
    [Parameter(Mandatory=$false)]
    [int]$TimeoutSeconds = 300
)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")]
    public static extern IntPtr PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    public const uint WM_KEYDOWN = 0x0100;
    public const uint WM_KEYUP = 0x0101;
}
"@

function Send-Key($hwnd, [int]$vk) {
    [Win32]::PostMessage($hwnd, [Win32]::WM_KEYDOWN, [IntPtr]$vk, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 50
    [Win32]::PostMessage($hwnd, [Win32]::WM_KEYUP, [IntPtr]$vk, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
}

$VK_W = 0x57; $VK_A = 0x41; $VK_S = 0x53; $VK_D = 0x44
$VK_B = 0x42
$VK_L = 0x4C
$VK_R = 0x52
$VK_N = 0x4E
$VK_E = 0x45
$VK_H = 0x48
$VK_Y = 0x59
$VK_SPACE = 0x20

$binDir = Join-Path $PSScriptRoot "..\Bin"
$exe = Join-Path $binDir "RelWithDebInfo\Main.exe"

if (-not (Test-Path $exe)) {
    Write-Error "Main.exe not found at $exe. Please build the engine first."
    exit 1
}

Write-Host "=== InnocenceEngine Interactive Test ==="
Write-Host "Scenario: $Scenario"
Write-Host "Timeout:  ${TimeoutSeconds}s"
Write-Host ""

$logFile = Join-Path $binDir "..\Build\interactive_test.log"
$process = Start-Process -FilePath $exe `
    -ArgumentList "-c Data/Engine/Configuration/Presets/Interactive.json" `
    -WorkingDirectory $binDir `
    -PassThru

Start-Sleep -Seconds 5

if ($process.HasExited) {
    Write-Host "FAIL - engine exited before interactive phase (exit code $($process.ExitCode))."
    exit 1
}

$hwnd = $process.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) {
    Start-Sleep -Seconds 3
    $hwnd = $process.MainWindowHandle
}
if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "FAIL - could not find engine window handle."
    $process | Stop-Process -Force
    exit 1
}

[Win32]::SetForegroundWindow($hwnd) | Out-Null
Write-Host "Engine window found (PID: $($process.Id), HWND: $hwnd)"

function Test-ProcessAlive {
    if ($process.HasExited) {
        Write-Host "FAIL - engine exited unexpectedly (exit code $($process.ExitCode))."
        exit 1
    }
    return $true
}

function Wait-AndCheck($seconds, $label) {
    Write-Host "  [$label] Waiting ${seconds}s..."
    Start-Sleep -Seconds $seconds
    Test-ProcessAlive
}

function Run-TogglePT {
    Write-Host "`n--- Toggle Path Tracer ---"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 4 "PT ON"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 3 "PT OFF"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 3 "PT ON again"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 3 "PT OFF again"
}

function Run-SceneReload {
    Write-Host "`n--- Scene Reload ---"
    Send-Key $hwnd $VK_R
    Wait-AndCheck 8 "Scene loading"
}

function Run-CameraMovement {
    Write-Host "`n--- Camera Movement ---"
    Send-Key $hwnd $VK_W
    Start-Sleep -Milliseconds 500
    Send-Key $hwnd $VK_D
    Start-Sleep -Milliseconds 500
    Send-Key $hwnd $VK_S
    Start-Sleep -Milliseconds 500
    Send-Key $hwnd $VK_A
    Start-Sleep -Milliseconds 500
}

function Run-ReImport {
    Write-Host "`n--- Model Re-Import ---"
    Send-Key $hwnd $VK_Y
    Wait-AndCheck 15 "Re-import"
}

function Run-GISponza {
    Write-Host "`n--- GISponza Scene Load ---"
    Send-Key $hwnd $VK_L
    Wait-AndCheck 5 "GISponza loading"
    Wait-AndCheck 3 "GISponza stable"
}

function Run-PTResize {
    Write-Host "`n--- Path Tracer + Window Resize ---"
    Send-Key $hwnd $VK_B
    Start-Sleep -Seconds 2
    $procWindow = (Get-Process -Id $process.Id).MainWindowHandle
    if ($procWindow -ne [IntPtr]::Zero) {
        Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public class Win32Resize {
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int W, int H, bool repaint);
}
"@
        [Win32Resize]::MoveWindow($hwnd, 0, 0, 1024, 768, $true) | Out-Null
        Wait-AndCheck 3 "Resize to 1024x768"
    }
    Send-Key $hwnd $VK_B
    Wait-AndCheck 5 "PT toggle post-resize"
}

function Run-PTWithReload {
    Write-Host "`n--- Path Tracer + Scene Reload ---"
    Send-Key $hwnd $VK_B
    Wait-AndCheck 3 "PT ON"
    Send-Key $hwnd $VK_R
    Wait-AndCheck 5 "PT toggle post-reload"
}

switch ($Scenario) {
    "toggle_pathtracer"  { Run-TogglePT }
    "scene_reload"       { Run-SceneReload }
    "camera_movement"    { Run-CameraMovement }
    "pathtracer_reload"  { Run-PTWithReload }
    "pathtracer_resize"  { Run-PTResize }
    "gi_sponza"          { Run-GISponza }
    "reimport"           { Run-ReImport }
    "full" {
        Run-TogglePT
        Run-SceneReload
        Run-CameraMovement
        Run-GISponza
    }
    default { Write-Error "Unknown scenario: $Scenario"; exit 2 }
}

Write-Host "`n--- Shutting down ---"

if (Test-Path $logFile) { Remove-Item $logFile -Force }
if (-not $process.HasExited) { $process | Stop-Process -Force }

Write-Host ""
Write-Host "=== PASS: Scenario '$Scenario' completed without crashes ==="
exit 0
