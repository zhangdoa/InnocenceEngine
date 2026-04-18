---
id: TASK-55
title: Path tracer entities without MaterialComponent silently get default white Lambert (no warning, no sentinel)
status: Done
assignee: []
created_date: '2026-04-17 16:55'
updated_date: '2026-04-18 07:55'
labels:
  - path-tracer
  - rendering
  - raytracing
  - materials
dependencies: []
priority: high
---

## Symptom

Running `Main.exe -offscreen -test gpu_path_tracer -capture_frame 30` produces a
rendered image where:

- Upper half: correct procedural sky gradient (miss shader working)
- Lower half: nearly flat light gray (no discernible geometry detail)

Readback stats: 921,600 pixels, all non-zero, `mean=(0.428, 0.455, 0.463)`,
`max=(0.803, 0.804, 0.810)`. RenderDoc capture confirms the `DispatchRays`
(PSO 927 `GPUPathTracerPass_RaytracingPSO`) executes at 1280×720 with 57 meshes
and 250 014 indices uploaded.

See `Build/captures/frame_pt.xml` (chunk 1435) for the captured dispatch.

## Investigation result

Initial hypothesis (zero material data for all instances) was **wrong**.
Per-instance logging in `RebuildGeometryBuffers` showed two groups:

- **Instances 0–9**: `matComp=0 matAsset=0`. Default-constructed
  `MaterialAttributes` gives `albedo=(1,1,1) metallic=0 roughness=1` — matte
  white Lambert. These entities have a mesh but no `MaterialComponent`.
- **Instances 10–56**: full material data — lambert white (0.7,0.7,0.7),
  metallic (0.8,0.8,0.8, metal=0.5), gold (1.0, 0.782, 0.344, metal=1),
  etc. Materials are reaching the GPU correctly.

The flat light-gray lower half in the output PNG is almost certainly a
large diffuse-white ground plane (one of instances 0–9) filling the lower
viewport and reflecting sky light. The 47 detailed shader balls and props
are probably outside the frame or small at the current camera position.

So the material-binding path works. The *remaining* concern is narrower:

**Entities with a mesh but no `MaterialComponent` silently get a white
Lambert fallback.** For the ShaderBall test scene this happens to look
fine, but it is an unenforced invariant: any entity can be TLAS-instanced
without a material, and the shader has no way to distinguish "no material"
from "white matte" — there is no default-material sentinel, no warning,
no scene-load assertion.

## Fix direction (reduced scope)

- Decide whether "mesh without material" is legal. If yes, formalise the
  default material (explicit `DefaultMaterial` constant, not implicit via
  struct zero-init). If no, assert/warn in `RebuildGeometryBuffers` and in
  the scene loader.
- Separate follow-up (lower priority): verify all 10 material-less
  entities are intentional (floor, walls, etc.) and not symptoms of a
  scene-import gap.

## How this was found

Via a programmatic RenderDoc capture + CPU readback pipeline added in this
session:
- Engine now loads `renderdoc.dll` via `LoadLibrary` fallback (works without
  `renderdoccmd` injection) — see
  `DX12GraphicsHardwareService::TryLoadRenderDocAPI`.
- `ExampleRenderingClient` now triggers a CPU readback of the final image
  after 30 frames when `-test gpu_path_tracer` is active, independent of
  `-total_frames`. Readback logs aggregate pixel stats
  (`PathTracerReadback: total=… zero=… nonZero=… mean=… max=…`) and writes
  `Bin/gpu_output.png`.
