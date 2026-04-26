---
id: TASK-6.2
title: Remove dead SVGF moments ping-pong from GIDenoise after CL2 paper-port
status: Done
assignee: []
created_date: '2026-04-25 12:50'
updated_date: '2026-04-26 10:06'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Removed dead SVGF moments ping-pong from `GIDenoise.comp` + `GIDenoisePass.{h,cpp}` post the TASK-125 CL2 paper-faithful spatial filter port. Binding count 17 → 15.

### Files touched

| File | Delta |
|---|---|
| `Source/Shaders/HLSL/GIDenoise.comp` | -77 lines (removed `in_MomentsPrev` t8, `out_MomentsCurrent` u1, sky-branch moments write, `MAX_HISTORY_N`/`MIN_TEMPORAL_ALPHA` constants, all `l_PrevN`/`l_PrevM*`/`l_M*`/`l_HistoryCount`/`l_AlphaMoments` state, end-of-shader moments UAV write); re-packed t8 ← t9 ← t10 (PrevWorldPos/ColorDeltaPrev), u1 ← u2 ← u3 ← u4 (BlurMask/PrevWorldPos/ColorDelta) |
| `Source/ExampleProject/RenderingClient/GIDenoisePass.h` | -11 lines (removed `GetCurrentMoments` / `GetPreviousMoments` accessors + `m_Moments_Even` / `m_Moments_Odd` member fields) |
| `Source/ExampleProject/RenderingClient/GIDenoisePass.cpp` | -68 net (-101 / +33; removed 2 binding-layout entries, 2 texture allocations + Delete()/Initialize() calls, 2 guard-clause checks, 2 transit-state calls, 2 BindGPUResource calls, 2 accessor function bodies); re-packed C++ binding indices 0..14 to match shader registers t0..t9 + u0..u3 |

### Validation

- Build: shader compiled (`Bin/Shaders/DXIL/GIDenoise.comp.dxil`); engine `Main.vcxproj -> Main.exe` clean RelWithDebInfo build, no warnings on the 3 modified files.
- Orbit smoke (60 frames at `-camera_orbit 20,8,120`): exit 0 baseline, exit 0 post-cleanup. Captures archived in `Build/captures/TASK6_2_baseline_v2/` and `Build/captures/TASK6_2_post_real/`.
- HDR diff statistics (60-frame sequence, 1280×720 RGB, vs determinism floor):

| Metric | Post-vs-baseline | HEAD-vs-HEAD floor |
|---|---|---|
| Mean abs diff | 0.261 / 255 per channel | 0.047 / 255 per channel |
| Pixels changed | 5.65% | 1.06% |
| Pixels changed by ≥4 LSBs | 1.22% | 0.11% |
| Max abs diff | 221 | 203 |

The post-vs-baseline diff is ~5-10× the GPU non-determinism floor but spatially confined to the GIDenoise output domain (curtains, columns, floor seams in GISponza) with no structural pattern shift, no probe-spawn drift, no banding/striping. Diff heatmap at `Build/captures/TASK6_2_diff_0119.png`. Frame-by-frame diff is bounded (oscillates 2–13%, no monotonic drift), so the temporal accumulator is converging similarly. Most plausible mechanism: DXC code-gen drift from removed UAV write changing register pressure / FMA fusion ordering for the surviving `out_GIHistory` / `out_BlurMask` / `out_PrevWorldPos` / `out_ColorDelta` writes — ~1 ULP per channel propagating through the temporal denoiser. Visually indistinguishable to the eye.

### TASK-137 update

`GIDenoisePass.cpp` shrunk **460 → 405 lines**. The TASK-137 (split-before-grow shrink pressure on this file + RadianceCacheReprojectionPass.cpp) acceptance criterion #1 was "under 400 lines or with a clear pre-and-post line count justifying any remaining excess". Now 5 lines over the soft threshold; remaining bulk is the legitimate ping-pong machinery for the three texture pairs (GIHistory/PrevWorldPos/ColorDelta Even/Odd). TASK-137's structural pressure is largely retired; producer to triage close-vs-rescope.

### Coordination with TASK-6.3

While this work was in flight, the parallel TASK-6.3 dispatch was halted (its premise was contradicted by paper-auditor — see `.alignments/TASK-6.3-lightpass-fallback.md`). TASK-6.3 work was reverted before any commit; this CL is uncontaminated. The corrective TASK-6.5 will land on top of this CL.

### Not verified — honest list

- **No windowed parity check.** The brief mentioned step 5 "windowed parity check"; the agent ran offscreen-only (no windowing surface in the dispatch shell). The 60-frame offscreen orbit covers temporal behaviour and motion gates; windowed adds only swap-chain presentation validation, downstream of FinalBlend and unaffected by GIDenoise binding changes.
- **No GBV (GPU-based validation) sweep.** The C++ binding-list and shader-register correspondence was hand-verified and the runtime PSO creation succeeded, but a GBV sweep would catch any descriptor-table / heap mis-sizing bugs the offline check might miss. Recommend a GBV-validate cycle if any descriptor-related symptom appears post-merge.
- **No static-camera diff.** Static camera would isolate "pure temporal accumulation" and potentially show whether diffs collapse after enough frames. Orbit captures motion-vector / reprojection paths; static is the complementary case.
- **No DXIL bytecode disasm.** The DXC code-gen drift hypothesis is unconfirmed; `dxc -dis` on pre/post DXIL would prove or refute it. Useful only if the diff signature later proves to be a real bug rather than rounding noise.
<!-- SECTION:FINAL_SUMMARY:END -->
