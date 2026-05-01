# Discipline: perf-measurement-frame-budget

Wall-clock budget drives `-total_frames N`, not "more frames is more accurate." Pick `N = max(10, ceil(target_wall_clock_ms / current_frame_ms))`. The GPU-timer ring is 3 frames, so N≥10 yields ~7 steady-state samples; going below 10 sacrifices ring warm-up, going above only buys diminishing returns.

| Purpose | Recommended N | Rationale |
|---|---|---|
| Perf isolation / measurement | `max(10, ceil(budget_ms / frame_ms))` | Budget-driven; ~7 steady samples once ring warms. |
| Smoke / regression "engine doesn't crash" | 30 | Not measurement; just confirm no fault path. |
| GBV / DX12 validation | 10 | Most validation issues fire in the first few frames. |
| Visual-correctness verification | per-task | Owner agent's `visual-validation.md` governs. |

Read the prior task's content rate before importing its N — copying N from a closed task without checking the perf bucket propagates a budget that may be 10× what the current bucket needs.

## Cross-references

- `regression-fix-flow.md` — paired; that one governs the bisect sequence, this one governs per-step cost.
- `test-etiquette.md` — both protect user attention against avoidable launches; this one governs N per launch, that one governs launch count.
- `visual-validation.md` — explicitly not governed here; visual ACs pick frame counts via that discipline.
