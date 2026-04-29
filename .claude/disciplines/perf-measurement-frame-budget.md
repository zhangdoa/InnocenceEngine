# Discipline: perf-measurement-frame-budget

Wall-clock budget drives `-total_frames N`, not "more frames is more accurate." Every Main.exe invocation that fronts perf isolation, regression smoke, or GBV check costs the dispatcher real bisect-step latency.

## Why

On 20-FPS content (e.g. GISponza), `-total_frames 100` is ~5 s wall-clock per step; the same N=10 is ~0.5 s. Across a 6-step bisect the difference is 30 s vs 3 s — and the agent ran the slow shape because it copied N from a closed task that ran on faster content. This is the perf-measurement analog of `regression-fix-flow.md`: that discipline is about action sequence (bisect before fix); this discipline is about per-step measurement budget (pick N for the perf bucket you are in).

## How

### Choosing N

Pick `N = max(10, ceil(target_wall_clock_ms / current_frame_ms))`.

- **GPU-timer ring is 3 frames.** N≥10 gives ~7 steady-state samples — enough for averaging without paying for slow content. Going below 10 sacrifices the ring-buffer warm-up; going above 10 only buys more samples, with diminishing returns.
- **Wall-clock budget is the constraint, not frame count.** A 0.5 s budget at 20 FPS is N=10. The same 0.5 s budget at 60 FPS is N=30. The same 0.5 s budget at 120 FPS is N=60. Pick budget first, derive N.

### By purpose

| Purpose | Recommended N | Rationale |
|---|---|---|
| Perf isolation / measurement | `max(10, ceil(budget_ms / frame_ms))` | Budget-driven; ~7 steady samples once ring warms. |
| Smoke / regression "engine doesn't crash" | 30 | Not measurement; just confirm no fault path. |
| GBV / DX12 validation | 10 | Most validation issues fire in the first few frames. |
| Visual-correctness verification | per-task | Owner agent's discipline (`visual-validation.md`) governs; not this discipline. |

### Wiring

The commit-gate `test-run` and `live-engine` block messages list `-total_frames N` as an example invocation; both cross-reference this discipline rather than naming a default N.

## Anti-patterns

- **Copying N from a prior closed task without checking the perf bucket.** Closed tasks frequently used 30 / 60 / 100 / 120 — most without recording the per-frame cost they ran against. Grepping precedent and copying N propagates a budget that may be 10x what the current bucket needs. Read the prior task's content rate before importing its N.
- **"More frames = more accurate."** Beyond ~7 steady-state samples (N≈10 with the 3-frame ring), additional frames buy variance reduction at linear cost. For bisect steps and gate-clearing runs, the variance is not the bottleneck — the wall-clock is.
- **Treating bisect-step cost as free.** Each step in a 6-step bisect compounds. A 5 s step is a 30 s loop; a 0.5 s step is a 3 s loop. The dispatcher's job per `regression-fix-flow.md` is to make each step cheap; N is the lever.
- **Picking N before knowing the content's frame time.** If the dispatcher does not know whether the scene runs at 20 FPS or 120 FPS, the brief should say "owner-agent picks N from this discipline" rather than naming a number.

## Recorded incident

User-flagged 2026-04-28: agents and the dispatcher were defaulting to `-total_frames 100` for perf measurement on slow content (GISponza ~20 FPS), making each bisect step ~5 s wall-clock instead of ~0.5 s. The pattern came from grepping closed-task precedent (most used 30 / 60 / 100 / 120 without context). Concretely: the TASK-169 brief specified `-total_frames 100`. With this discipline in place, the same brief would specify `-total_frames 10` (or defer the choice to the owner agent referencing this file).

## Cross-references

- `regression-fix-flow.md` — paired discipline; that one governs the bisect *sequence*, this one governs per-step *cost*. Together: bisect first, and make each bisect step cheap.
- `test-etiquette.md` — both disciplines protect user attention against avoidable Main.exe / editor launches; this one governs N per launch, that one governs launch count.
- `visual-validation.md` — explicitly not governed here; visual ACs pick frame counts via that discipline.
