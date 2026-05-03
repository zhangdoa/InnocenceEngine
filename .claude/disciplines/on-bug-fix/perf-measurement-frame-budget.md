# Discipline: perf-measurement-frame-budget

Wall-clock budget drives `-total_frames N`, not "more frames is more accurate." The GPU-timer ring is 3 frames, so `N ≥ 10` yields ~7 steady-state samples.

Pick `N = max(10, ceil(target_wall_clock_ms / current_frame_ms))`. Going below 10 sacrifices ring warm-up; going above buys diminishing returns.

| Purpose | N |
|---|---|
| Perf isolation / measurement | `max(10, ceil(budget_ms / frame_ms))` |
| Smoke / regression "engine doesn't crash" | 30 |
| GBV / DX12 validation | 10 |
| Visual-correctness verification | per-task; `on-rendering/visual-validation.md` governs |

Read the prior task's content rate before importing its N. Copying N from a closed task without checking the perf bucket can propagate a budget 10× what's needed.

## Cross-references

- `on-bug/regression-fix-flow.md` — paired: governs the bisect sequence.
- `on-test/test-etiquette.md` — protects user attention via launch count.
- `on-rendering/visual-validation.md` — visual ACs pick frame counts via that discipline.
