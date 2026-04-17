---
id: TASK-56
title: Investigate GISponza scene not converging / noise drifts under path tracer
status: Todo
assignee: []
created_date: '2026-04-17 17:45'
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
