# Kill stray engine processes before a fresh launch.
# Run before invoking Main.exe / RenderTest.exe to avoid contending with a hung instance.
$names = 'Main', 'RenderTest', 'InteractiveTest'
$procs = Get-Process -Name $names -ErrorAction SilentlyContinue
if (-not $procs) {
    Write-Host 'No orphans found.'
    exit 0
}
foreach ($p in $procs) {
    Write-Host "Killing $($p.ProcessName) PID $($p.Id)"
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
}
