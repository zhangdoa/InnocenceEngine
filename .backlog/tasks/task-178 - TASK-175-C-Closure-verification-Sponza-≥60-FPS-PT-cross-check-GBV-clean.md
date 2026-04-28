---
id: TASK-178
title: 'TASK-175-C: Closure verification — Sponza ≥60 FPS, PT cross-check, GBV clean'
status: To Do
assignee: []
created_date: '2026-04-28'
labels:
  - rendering
  - shadows
  - performance
  - verification
dependencies:
  - TASK-176
  - TASK-177
parent_task_id: TASK-175
priority: high
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/ExampleProject/RenderingClient/LightPass.cpp
  - Source/ExampleProject/RenderingClient/GPUPathTracer.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask C of TASK-175 (unify shadow paths under RT).** Owner: `rendering-researcher`.

Final closure verification. Runs after TASK-176 (replacement) and TASK-177 (deletion) both land. Produces the perf + visual evidence that closes the parent TASK-175.

### What this subtask delivers

1. **GPU-timer perf measurement on Sponza windowed**:
   - `PointShadow*` timer entries gone (TASK-177 deleted the passes; verify timer registry).
   - `LightPass` cost in 1-2 ms expected range (per TASK-175 spec cost estimate).
   - Total frame ≤16.67 ms for 60 FPS bar; target ~10 ms per spec.
   - **N choice per `perf-measurement-frame-budget.md`**: at the new ~60+ FPS rate, `N = max(10, ceil(500 / 16.67)) = 30`. Use `-total_frames 30` for the perf measurement run. Do NOT copy `-total_frames 100` from prior closed tasks (lesson 2026-04-28).
   - Methodology: `Main.exe -mode 0 -renderer 0 -total_frames 30` on `GISponza`, capture timer output, average across the steady-state samples (frames 4-30 once the GPU-timer ring warms).

2. **Path-tracer cross-check**:
   - Run `GPUPathTracer` (existing) on the same scenes (`GISponza` + the UnitTest sphere geometry).
   - Compare RAST + inline-RT shadow output to PT reference *for the shadow term*. Soft shadows for sphere lights should match the PT reference within visual tolerance (cone-jittered single-frame RAST + TAA temporal accumulation should converge to the PT ground truth on static frames).
   - Document any deltas. If the delta is large, the inline trace's sampling shape is wrong — bisect back into A; do NOT close C with deltas unexplained.
   - **Anchor lesson `feedback_pt_comparison_must_account_for_rast_omissions.md`**: when comparing PT-vs-rast, enumerate what rast does not compute (e.g. multi-bounce indirect, caustics, light types not implemented in rast) before drawing conclusions. The shadow term is the apples-to-apples surface; everything else is gap-by-design.

3. **GPU-based validation pass**:
   - `Main.exe -gpu_validation -total_frames 10` on `GISponza`.
   - Pre-existing TASK-163 readback ERROR is the only acceptable message.
   - Any new ERROR/WARNING is a defect — bisect to A or B and fix.

4. **30-second walkthrough on Sponza windowed**:
   - Manual driving session (camera moving through the scene, all 8 point/sphere lights visible at various angles).
   - Sustained ≥60 FPS over the 30s — no frame-time spikes above 16.67 ms.
   - User-attended (zhangdoa) sign-off, since this is the user-flagged headline result.

### What this subtask delivers as artifacts

- A **`Reviewed-By:` annotated commit** (per `peer-review-required.md`) with measurement results in the message body — even though this CL may be backlog-only, it carries the closure claim and warrants review.
- TASK-175 parent's Final Summary updated with the perf delta (before vs after), visual evidence pointers, and the GBV result.
- TASK-175 status flipped to `Done`.

### SOTA tech-choice anchor (mandatory)

For closure measurement: justify the verification approach against (a) training-default ("run frametime monitor for 30s"), (b) current SOTA ("perfetto trace + flame graph + GPU breakdown"), (c) what the project does (engine GPU-timer registry + RenderDoc + PT cross-check). Pick (c) — same engine, same instrumentation, same precedent (TASK-138 closure). The engine GPU-timer is already wired and runtime-gated post-TASK-165.

### Project invariants (anchor — read before measuring)

1. **60-FPS bar (rendering-researcher manifest, 2026-04-27)** — load-bearing for this task. If post-A+B Sponza is not ≥60 FPS, the parent TASK-175 is NOT closed; reopen A for tuning.
2. **TASK-149**: editor flag still toggles shadow-casting; manual flip during walkthrough verifies end-to-end (camera at a position where toggling a light's `m_CastShadow` produces an obvious visible change).
3. **No log-spam**: walkthrough run at default log level produces no per-frame Verbose channel firehose (post-TASK-165).
4. **`perf-measurement-frame-budget.md`**: `N=30` for the perf run, NOT `N=100`.

### What this subtask does NOT do

- No code changes (this is verification-only). If any AC fails, file a follow-up task or re-open A/B; do NOT patch in the closure CL.
- No further deletions — TASK-177 is the deletion authority.
- Does not decide whether `SunShadowRTPass` folds into `lightPass.comp`. That is a separate design decision (file as TASK-179 if pursued).

### Validation

- Perf evidence captured (timer dump, total frame ms, per-pass breakdown).
- PT cross-check evidence (screenshot pair or pixel-diff metric).
- GBV evidence (clean-modulo-TASK-163 log).
- 30s walkthrough evidence (FPS-over-time observation; no spikes).
- All 8 ACs of parent TASK-175 satisfied with line-grounded evidence in the parent's Final Summary.

### Peer review

Per `peer-review-required.md`: closure CL is substantive (carries the closure claim). Reviewer: `software-architect` (cross-role structural read on whether the parent's ACs are *actually* met by the cited evidence — bias toward "this evidence does not support that claim", same posture as the verdict-skepticism in `paper-audit.md`). If `software-architect` is not the right venue, fall back to peer `rendering-researcher`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 GPU-timer dump shows `PointShadow*` entries gone; `LightPass` in 1-2 ms range
- [ ] #2 Sponza windowed sustained ≥60 FPS over 30s walkthrough (user-attended)
- [ ] #3 PT cross-check: rast + inline-RT shadow term matches PT reference within tolerance for sphere/point/spot lights
- [ ] #4 GBV clean (modulo pre-existing TASK-163 readback ERROR); no new ERROR/WARNING
- [ ] #5 Editor `m_CastShadow` flip during walkthrough produces expected visible change (TASK-149 end-to-end)
- [ ] #6 Parent TASK-175 Final Summary updated with perf delta, visual evidence, GBV evidence
- [ ] #7 TASK-175 status flipped to `Done`
- [ ] #8 Closure CL reviewed by `software-architect` (or peer `rendering-researcher`) before commit
- [ ] #9 N=30 used for perf run (NOT N=100); justification cited per `perf-measurement-frame-budget.md`
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
<!-- SECTION:NOTES:END -->
