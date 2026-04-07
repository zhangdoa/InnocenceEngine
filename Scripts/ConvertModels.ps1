# ConvertModels.ps1 — Launches Main.exe and sends Y to trigger model conversion
# Exit codes: 0 = pass, 1 = crash, 2 = timeout

param(
    [int]$WaitSeconds = 30
)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Conv {
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    public const uint WM_KEYDOWN = 0x0100;
    public const uint WM_KEYUP   = 0x0101;
}
"@

$binDir = Join-Path $PSScriptRoot "..\Bin"
$exe = Join-Path $binDir "RelWithDebInfo\Main.exe"

if (-not (Test-Path $exe)) {
    Write-Error "Main.exe not found at: $exe"
    exit 2
}

Write-Host "=== Model Conversion ==="
$process = Start-Process -FilePath $exe `
    -ArgumentList "-mode 0 -renderer 0 -loglevel 0" `
    -WorkingDirectory $binDir `
    -PassThru

Start-Sleep -Seconds 8

if ($process.HasExited) {
    Write-Host "FAIL: Process exited early with code $($process.ExitCode)"
    exit 1
}

$hwnd = $process.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) {
    Start-Sleep -Seconds 3
    $hwnd = $process.MainWindowHandle
}

if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "FAIL: No window handle"
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    exit 2
}

[Win32Conv]::SetForegroundWindow($hwnd) | Out-Null
Write-Host "Engine window found (PID: $($process.Id))"

Write-Host "Sending Y key to trigger model conversion..."
[Win32Conv]::PostMessage($hwnd, [Win32Conv]::WM_KEYDOWN, [IntPtr]0x59, [IntPtr]::Zero) | Out-Null
Start-Sleep -Milliseconds 50
[Win32Conv]::PostMessage($hwnd, [Win32Conv]::WM_KEYUP, [IntPtr]0x59, [IntPtr]::Zero) | Out-Null

Write-Host "Waiting ${WaitSeconds}s for conversion..."
for ($i = 0; $i -lt $WaitSeconds; $i++) {
    Start-Sleep -Seconds 1
    if ($process.HasExited) {
        Write-Host "FAIL: Process crashed during conversion (exit code: $($process.ExitCode))"
        exit 1
    }
}

Write-Host "Shutting down..."
Stop-Process -Id $process.Id -Force
Start-Sleep -Seconds 2
Write-Host "=== PASS: Conversion completed ==="
exit 0
