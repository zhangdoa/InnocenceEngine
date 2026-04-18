---
id: TASK-56
title: Investigate GISponza scene not converging / noise drifts under path tracer
status: Done
assignee: []
created_date: '2026-04-17 17:45'
updated_date: '2026-04-18 18:47'
labels:
  - path-tracer
  - scene-data
  - sponza
dependencies: []
priority: medium
---

## Symptom

Running the path tracer with `-test gpu_path_tracer` renders
`UnitTest.ShaderBall.InnoScene`, which includes `NewSponza_Curtains_glTF`
as a child scene. The resulting image does not converge over time — the
noise pattern drifts (user described "pixels flying towards left")
despite the path tracer's accumulation logic being sound (UnitTest
standalone converges cleanly in the same session).

## Contrast

- **UnitTest scene (standalone)**: path tracer converges quickly,
  produces clean output. Confirmed by user.
- **GISponza / NewSponza**: does not converge, keeps changing each frame.

## Likely causes to investigate (in order of probability)

1. **Something in the Sponza scene animates** — a bone transform, a
   light pose, a tick-driven value. The accumulation reset gate in
   `GPUPathTracerPass::Update` is *view-matrix* only:
   `if (memcmp(&l_perFrameCB.v, &m_PrevViewMatrix, sizeof(Mat4)) != 0) reset;`
   If the view matrix is stable but some other driver-visible state
   (light position, material value, TLAS instance transform) changes per
   frame, accumulation runs but each frame contributes a different
   reference, producing a drift.
2. **TLAS instance transforms recomputed each frame** with floating-point
   jitter — e.g. if a `TransformService` tick writes back a matrix that
   differs in the last-bit. Would cause each ray to hit slightly
   different positions, producing per-frame drift without a "reset"
   signal.
3. **Unintended camera motion via Player::Update** — `m_IsTP = true` in
   non-editor mode; Player::Update only writes to camera transform
   "if (m_IsTP && l_playerTransform)". The animation controller's
   Simulate loop may incidentally tick transforms that propagate to the
   camera path.

## Verification steps

- Add one-line `Log(Verbose, "PathTracer: viewHash=", hash, " frameCount=", m_FrameCount)`
  in `GPUPathTracerPass::Update` and run for 30 frames. If frame count
  resets every frame, view matrix is changing → look at camera driver.
  If frame count grows but output still drifts, it's not the view
  matrix — look at other per-frame state (TLAS transforms, light params).
- Dump bound resources at an `DispatchRays` capture and diff between
  two consecutive frames.

## Out of scope here

The path tracer accumulation math itself is correct — verified against
UnitTest. This task is strictly about what in the Sponza scene setup
makes it behave as if it's a moving scene.

## Investigation progress (2026-04-18)

Diagnostics landed in 3b99dafc:
- `GPUPathTracerPass::Update` logs accumulation resets.
- `DX12GPUBufferResourceService::UpdateRaytracingInstances` logs TLAS
  rebuilds with dirty-transform count.

Results over 40 frames with `-test gpu_path_tracer`:

- **View matrix is stable.** Only two resets: frame 0 (initial) and
  frame 7 (view re-evaluated shortly after GISponza finishes loading).
  From frame 7 onwards the accumulator grows monotonically.
- **TLAS is stable.** Three rebuilds: frame 0 (initial, 57 instances),
  frame 5 (GISponza load brings count to 94), frame 6 (GISponza's
  instances dirty once after load). Stable after frame 6 — no per-frame
  matrix jitter from `TransformService`.

So the drift is not "camera moved" and not "TLAS changed". Remaining
suspects in order of likelihood:

1. **Sub-pixel jitter sequence (expected behaviour, perceived as
   drift).** `GenerateCameraRay` adds `Halton(g_FrameCount, 2)` /
   `Halton(g_FrameCount, 3)` offsets. Each frame samples a different
   sub-pixel location — this *is* how progressive anti-aliasing works,
   and visible "pixels moving" at low sample counts is expected.
   Convergence rate depends on scene variance.
2. **Light / material buffer updates not yet traced.** If
   LightDataService or materials re-upload per frame with FP jitter,
   each sample integrates slightly different lighting.
3. **BVH builder non-determinism across sessions** (not intra-session
   after frame 6).

Next concrete step: save output PNG at frame 30 and frame 100, diff
pixel-wise. If differences shrink roughly with 1/N, convergence is
fine and the user report is "jitter visible at low sample counts",
not a bug. If differences have spatial or temporal bias, investigate
light buffers and per-bounce RNG.

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Investigation concluded — the path tracer DOES converge on GISponza; the reported "drift / pixels flying left" is the Halton sub-pixel jitter sequence in `GenerateCameraRay`, which IS how progressive AA works. Verified across 30 / 100 / 300 frame runs:

- Accumulation reset gate fires only at frame 0 + frame 7 (right after GISponza load), then monotonic — view matrix IS stable.
- TLAS rebuilds only at scene-load boundaries, not per frame — no FP jitter in instance transforms.
- Zero-pixel count drops 53k → 6.7k → 0.7k (6% → 0.7% → 0.08%) as more paths find light.
- Mean brightness stabilises in the 0.4-0.55 range; the 100→300 change is within expected noise for a 4-bounce 1spp Sponza interior.
- Image is recognisably Sponza at all three counts, with less noise at higher counts.

Slow convergence on GISponza is inherent to a dark interior scene with 4 bounces and 1 sample/pixel/frame; light mostly arrives via indirect bounces. Work that improved this during the session: TASK-19 (bindless albedo), TASK-69 (back-face normal flip so curtains don't reflect as chrome), TASK-67 (sphere-light area sampling). Further quality gains belong to separate tasks (MIS between BRDF and light sampling, light BVH for many-light scenes, reconstruction filter / denoiser, stratified sampling per pixel) rather than this investigation.
<!-- SECTION:FINAL_SUMMARY:END -->
