---
id: TASK-6.2
title: Remove dead SVGF moments ping-pong from GIDenoise after CL2 paper-port
status: To Do
assignee: []
created_date: '2026-04-25 12:50'
labels:
  - rendering
  - GI
  - cleanup
  - tech-debt
dependencies: []
references:
  - Source/Shaders/HLSL/GIDenoise.comp
  - Source/ExampleProject/RenderingClient/GIDenoisePass.h
  - Source/ExampleProject/RenderingClient/GIDenoisePass.cpp
parent_task_id: TASK-6
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Description

The §2.4.3 paper-faithful spatial filter shipped in TASK-125 CL2 no longer reads the SVGF moments texture (E[L], E[L²], N) — the variance-driven luminance edge-stop was Schied 2017 SVGF, not part of the GI-1.0 paper. The new H/V bilateral filter uses depth + normal weights only.

The moments texture and ping-pong (`m_Moments_Even` / `m_Moments_Odd` in `GIDenoisePass`) were intentionally left in place during CL2 to keep the binding-set diff small and avoid reshuffling descriptor indices alongside a substantial pipeline rewrite. They are dead weight in the post-CL2 pipeline:

- `GIDenoise.comp` still computes and writes the (E[L], E[L²], N) values to `out_MomentsCurrent` (u1).
- The two ping-pong textures `m_Moments_Even` / `m_Moments_Odd` are still allocated and rotated.
- Nothing reads them — the filter passes consume only `GIHistory` (CL1) and `BlurMask` (CL2).

## Cleanup

1. Remove `out_MomentsCurrent` (u1) and `in_MomentsPrev` (t8) bindings from `GIDenoise.comp`.
2. Remove the moments-related state computation (`l_PrevN`, `l_PrevM1`, `l_PrevM2`, `l_M1`, `l_M2`, `l_HistoryCount`, `MAX_HISTORY_N`, `MIN_TEMPORAL_ALPHA`) — none of it feeds anything else.
3. Remove `m_Moments_Even` / `m_Moments_Odd` and the `GetCurrentMoments` / `GetPreviousMoments` accessors from `GIDenoisePass`.
4. Re-pack descriptor indices (t8 → free) and adjust the resource binding layout from 17 → 15.
5. Re-run TASK-124 orbit smoke + windowed parity check.

Comment markers in the CL2 source pointing at this cleanup:

- `Source/Shaders/HLSL/GIDenoise.comp` — "Dead under the §2.4.3 paper-faithful filter…tracked as a CL2 follow-up cleanup"
- `Source/ExampleProject/RenderingClient/GIDenoisePass.h` — "Dead under the §2.4.3 paper-faithful filter — kept on this CL to keep the shader binding-set diff small"

## Why deferred from CL2

Tightening descriptor packing during a paper-port migration risks introducing a subtle binding-mismatch bug in the same CL that's already replacing three shader programs and the pipeline wiring. Bisect surface stays cleaner if this lands as a separate code-only cleanup, with the rendering behaviour identical before/after.

## Definition of Done
<!-- DOD:BEGIN -->
Adopts project defaults.
<!-- SECTION:DESCRIPTION:END -->

- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
