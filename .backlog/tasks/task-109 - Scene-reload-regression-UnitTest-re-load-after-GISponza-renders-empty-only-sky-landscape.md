---
id: TASK-109
title: >-
  Scene reload regression: UnitTest re-load after GISponza renders empty (only
  sky + landscape)
status: To Do
assignee: []
created_date: '2026-04-19 19:39'
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
