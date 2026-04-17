---
id: TASK-55
title: Path tracer ShaderBall scene shades with zeroed material data, causing flat gray output
status: Todo
assignee: []
created_date: '2026-04-17 16:55'
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

## Hypothesis

The closest-hit shader reads per-instance material data from
`in_MaterialBuffer[InstanceID()]`. In
`GPUPathTracerPass::RebuildGeometryBuffers` the entry is constructed as:

```cpp
MaterialConstantBuffer l_materialCB = {};  // zero-init
auto* l_matComp = l_registry->Get<MaterialComponent>(l_entity);
if (l_matComp)
{
    auto* l_matAsset = AssetService::GetMaterialAsset(l_matComp->m_Asset);
    if (l_matAsset)
        l_materialCB.m_MaterialAttributes = l_matAsset->m_Attributes;
}
```

If either pointer is null — which is probable for entities that have a mesh
but no material component, or whose material asset handle is stale after a
scene reload — the entry stays zero-initialised. The shader then receives
`albedo=(0,0,0)`, `metalness=0`, `roughness=0` (clamped to 0.04 in the ray
gen). With `albedo=0`, the Disney diffuse term evaluates to zero and only the
specular lobe contributes — producing a near-mirror reflection of the sky that
looks like a flat wash rather than a recognisable scene.

## Verification steps

1. Log per-instance `(albedo, metalness, roughness, hasMatAsset)` in
   `RebuildGeometryBuffers` — confirm the distribution of non-zero materials.
2. Dump the mega-material buffer contents during a RenderDoc capture; the
   buffer is bound as SRV `t1` (descriptor table at root parameter 3).
3. If most entries are zero, trace why: (a) missing `MaterialComponent`,
   (b) stale `MaterialAssetHandle`, (c) asset not loaded at time of rebuild.

## Fix direction

- If material is absent, use a sensible fallback (e.g. white Lambert) instead
  of zero, so hit surfaces are at least visible.
- Rebuild geometry buffers again once material assets become resident (same
  pattern as `AreMeshesGPUReady`, but for materials).
- Consider adding an `AreMaterialsGPUReady` gate and re-triggering
  `m_PendingGeometryRebuild` when a previously-missing material becomes
  resident.

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
