# Pick a `-total_frames N` value for a perf / smoke run.
# N = max(10, ceil(budget_ms / current_frame_ms))
# GPU-timer ring is 3 frames, so N>=10 yields ~7 steady-state samples.
param(
    [Parameter(Mandatory)][int]$BudgetMs,
    [Parameter(Mandatory)][double]$FrameMs
)
if ($FrameMs -le 0) { throw 'FrameMs must be > 0' }
$N = [Math]::Max(10, [int][Math]::Ceiling($BudgetMs / $FrameMs))
Write-Output $N
