---
id: TASK-109
title: >-
  Scene reload regression: UnitTest re-load after GISponza renders empty (only
  sky + landscape)
status: To Do
assignee: []
created_date: '2026-04-19 19:39'
updated_date: '2026-04-19 20:55'
labels:
  - bug
  - rendering
  - scene-reload
  - regression
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

The tier-3 scene-reload test (`-total_frames 20 -reload_at_frame 10`) ends on a UnitTest render that shows only sky and a flat off-white landscape plane — all the primitives, the shader ball, the material sphere grid, the lights have visually disappeared. The engine logs show scene-load and mesh BLAS initialization fire cleanly, so the data path reaches the GPU, but the rasterized draw never shows the geometry. User report also mentions "shrunk shader ball, extra bright light at that position, flashing shadows" which would be consistent with a draw-call / transform / light state corruption on reload rather than full omission — the offscreen capture may simply happen to land at a frame where the geometry is subpixel.

## Reproduction

```
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10' -Wait -PassThru -NoNewWindow).ExitCode"
```

Then view `Bin/gpu_output.png`. Expected: the UnitTest scene with prim shapes + material sphere grid + shader ball + lights. Actual: empty sky + flat ground.

tier-2 (single GISponza load, no reload) renders correctly; tier-3 (UnitTest → GISponza reload at 5 → UnitTest reload at 10) breaks.

## Confirmed NOT the cause

- Data edit in `00ba9639` (Player Character removal) — reproduces against the pre-edit scene version from commit `91b196c0`.
- Bunny / dragon PBR material changes from TASK-102.
- Path tracer sky NEE from TASK-94.

## Investigation angles

- **Accumulation bug on mesh / material asset reuse**: first UnitTest load creates BLAS for UnitSquareMesh/UnitCubeMesh/etc.; GISponza reload may or may not destroy them; UnitTest re-load picks up stale handles.
- **Camera transform not re-hydrated on reload**: the camera may end up pointing away from the scene (all primitives off-screen).
- **Light state corruption**: "extra bright light, flashing shadows" suggests a LightComponent with stale transform / intensity that changes frame-to-frame.
- **Draw-call submission buffer**: DrawCallService may accumulate entries from previous scenes if its OnSceneUnloading is missing a clear.
- **TLAS rebuild**: path tracer works at tier-2 on the first-loaded scene; not verified post-reload.

## Related

- TASK-29 (done) — promoted reload test to standard gate. It's been passing exit-code-0 but exit-code doesn't cover visual correctness.
- TASK-5 (done, high) — earlier path tracer reload crash; this is the rasterizer counterpart.
- CLAUDE.md notes "Scene reload ... catches device-removed crashes from in-flight resource destruction, stale descriptors, and multi-load accumulation bugs." This is exactly that failure class but the current gate only catches the *crash* mode, not the *silently-wrong* mode.

## Acceptance

- `gpu_output.png` from the tier-3 reload test shows the UnitTest scene's primitives and material sphere grid at their authored transforms.
- No "extra bright light" flickering or flashing shadows between successive frames after reload.
- Add a visual-correctness assertion to the reload gate — e.g. a pixel-sample check that confirms at least one non-sky / non-ground pixel is present in the output.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**User update (2026-04-19 22:15)**: the shader ball renders *differently every launch* — "sometimes it's fine, sometimes a part of it is missing, and sometimes it's gone." That's non-determinism across fresh Main.exe processes, not just the reload case. Pattern strongly suggests: uninitialized memory read, unsynchronised BLAS/TLAS build vs. first draw, or a data race between deferred resource initialization and the first frame. Investigation should start by running the same scene N times and collecting gpu_output.png from each — if any two differ despite identical input, the draw path has a race. Candidate culprits: ShaderBall.0-4.MeshComponent use deferred BLAS init; if the first frame's TLAS build races the last BLAS init, that specific mesh's geometry is garbage for that frame. Check for a WaitForGPU / fence between "ProcessDeferredMeshInit" and "BuildTLAS".

**Investigation 2026-04-19 22:35** (Opus): reproduced the non-determinism with a 3-launch loop against UnitTest at `-total_frames 4` — got 3 different md5 hashes on gpu_output.png. Hypothesis "mesh init races with first draw" was addressed by commit `0498fa71` (drain deferred queues + WaitForGPUIdle inside LoadSync before rendering resumes), but re-running 3 times post-fix still produced 3 different hashes. So the race is NOT in mesh/texture/material/GPU-buffer deferred init — those are now fully drained before the first post-load frame.

Remaining suspects (in rough priority):
1. **DrawCallService buffer ordering** — if parallel tasks upload GPU buffers out-of-order relative to the draw submit, per-frame constants could be stale. DrawCallService uses a mutex but tasks that feed it may not.
2. **LuminanceHistogram / auto-exposure convergence** — first few frames not converged; if any compute dispatch orders differ, exposure varies.
3. **TLAS build timing** — for path tracer specifically, though user reports rasterizer also varies.
4. **Thread-scheduling of per-entity draw-call population** — if any RegisterDrawCall-equivalent fires from multiple threads without deterministic ordering.

Next step would be RenderDoc captures of two different runs and diffing the draw-call sequence. That's a substantial investigation — probably a ~1 day task on its own.

**Cross-reference 2026-04-19 22:56** (Opus): the TASK-111 scaffolding attempt surfaced that `MeshResourceService::OnSceneUnloading`'s filter (`GetLifespan(owner) == Scene`) does NOT reliably drop `ShaderBall.0.MeshComponent` from the deferred init queue — the task survives into the next scene's init pass with a dangling asset handle. This is likely the SAME race that causes the launch-to-launch shader ball non-determinism: whether ShaderBall's task gets drained on time depends on thread scheduling. OnSceneUnloading's filter needs scrutiny — either `GetLifespan` returns something other than `Scene` for child-scene entities, or there's a window where the owner EntityID is valid-but-not-yet-registered, or the task is being push()'d AFTER OnSceneUnloading ran. Each deserves a direct test.
<!-- SECTION:NOTES:END -->
