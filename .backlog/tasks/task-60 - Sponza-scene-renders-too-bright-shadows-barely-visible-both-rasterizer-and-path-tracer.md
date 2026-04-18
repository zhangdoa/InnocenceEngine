---
id: TASK-60
title: >-
  Sponza scene renders too bright / shadows barely visible (both rasterizer and
  path tracer)
status: Done
assignee: []
created_date: '2026-04-18 09:50'
updated_date: '2026-04-18 11:29'
labels:
  - bug
  - rendering
  - sponza
  - lighting
  - scene-data
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User report (2026-04-18): when the auto-test loads `GISponza.InnoScene`, the output looks uniformly bright — curtain folds and pillar bases that should be in shadow are not visibly darker. The noise pattern is also strong enough that geometry is visible but contrast is flat.

**Key facts:**
- Same symptom in both the rasterizer and the GPU path tracer — not a path-tracer-only bug.
- `UnitTest.InnoScene` renders correctly in the same session (shadows and lighting look right).
- Hypothesis: Sponza scene's light intensities / RGB values / material values don't match the exposure + tonemap assumptions the engine's pipeline is calibrated for. The Sun JSON has `LuminousFlux = 100000`, same as UnitTest's — so the problem is more likely in *where* the light points, material range, or the combination of Sponza-sized geometry with the physical-camera exposure calc in `GPUPathTracerToneMap.hlsl`.

**Next investigation steps:**
1. Load `GITestBox` as a third reference (different intensities/layout) and compare output to isolate whether the problem is scene-data or pipeline.
2. Dump the tonemap input (AccumBuffer or GBuffer-light result) pixel statistics for Sponza vs UnitTest — if Sponza's raw HDR is all saturating the ACES knee, exposure is the issue.
3. Check Sponza's sun direction — if the sun points away from the scene or shadows land outside the frustum, fill lighting would look uniform.
<!-- SECTION:DESCRIPTION:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Investigation 2026-04-18

Audit dump of GISponza at frame 30 (`-total_frames 30 -audit`) narrowed the cause to the **sun shadow map** — the lit illuminance buffer is uniform because its shadow factor is ~0 everywhere there's no ground-plane occlusion.

Shadow-map pixel coverage per cascade:
- Cascade 0 (nearest): 100% non-cleared
- Cascade 1: 99.66%
- Cascade 2: 76.55%
- Cascade 3 (widest): **4.54%** non-cleared

Visually the shadow map stores the ground plane's linear depth gradient in cascades 0–2 and almost nothing in cascade 3. Curtain and pillar silhouettes — the occluders the user expects to cast shadows — never appear.

World-space AABBs per cascade (from the new Verbose log in `LightDataServiceImpl::UpdateCSMData`):
- Cascade 0: X [-41, 41], Y [-21, 25], Z [-41, -0.001]
- Cascade 1: X [-82, 82], Y [-44, 48], Z [-82, -0.001]
- Cascade 2: X [-138, 138], Y [-76, 80], Z [-138, -0.001]
- Cascade 3: X [-657, 657], Y [-367, 371], Z [-657, -0.001]

GISponza geometry reaches posX = 188 and posZ = 128, so only cascades 2 and 3 can contain it. Cascade 3 *does* contain it but the curtains and pillars aren't being written into the shadow RT0 (VSM depth buffer).

`SunShadowResolver` in `shadowResolver.hlsl` returns 0.0 (no shadow) if the shaded pixel's `positionWS` doesn't fall inside any cascade's world AABB — that part is fine. The failure upstream: the shadow *geometry* pass isn't rendering curtain/pillar depth into the shadow atlas.

Candidates to investigate next:
- Pipeline state on the shadow pass: viewport/scissor matching the cascade slice in the texture-array target?
- Geometry shader `sunShadowGeometryProcessPass.geom` — triangles projected for cascade 3 might be falling entirely outside the orthographic frustum due to wrong matrix multiplication order or a `/w` that breaks for orthographic where w should be left alone (note: `output.posCS /= output.posCS.w;` in the GS — harmless for w=1 ortho, but worth re-verifying with debug output).
- Whether the shadow pass's PSO enables back-face culling in a way that eliminates thin two-sided curtain meshes.

Related: the `-total_frames ... -total_frames N` launches are also hitting TASK-39 (startup AV in `Engine::Get<LogService>`) roughly 1-in-3 runs — second invocation always worked. Not blocking TASK-60 but worth noting the repro finally landed.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Two-part fix landed in bc6aa204 + 1734baaa:

1. **Root cause**: `SunShadowGeometryProcessPass` used `RasterizerCullMode::Front` (Peter-Panning mitigation), but Sponza has many thin single-sided meshes (curtains, cloth) whose every triangle faces one way relative to the sun. Front-face culling eliminated them from the shadow map entirely, so cascades 0-2 stored only the ground plane's depth gradient and cascade 3 was 95% empty. Every shaded pixel sampled "no occluder", shadow factor was 0, lighting was uniform.

   Fix: disable back-face culling in the shadow pass. Peter-Panning is handled by the shader's adaptive `MIN_SHADOW_BIAS`/`MAX_SHADOW_BIAS` in `shadowResolver.hlsl` — the hardware cull mode was belt-and-suspenders.

   Post-fix cascade coverage went from (100%/99%/76%/4.5% mostly-ground) to (~90%/94%/76%/4.5% with actual occluders).

2. **Scoped**: radiance-cache GI passes temporarily disabled so the per-pixel noise doesn't mask direct-lighting debugging. Tracked as "re-enable once TASK-6 / GI quality lands".

**Visible result**: Sponza now shows clear curtain self-shadowing, pillar directional lighting, ground shadows from occluders, dark ceiling where sun doesn't reach.

Regression tiers pass: RenderTest, Main 10-frame integration, scene reload.

Related tasks that surfaced during investigation:
- TASK-39 got a fresh repro note (startup AV hit 1-in-3 invocations)
- The "max Y = 0" artifact in audit dump HDRs turned out to be stb_image_write clamping negatives — dump-format limitation, not a real bug. Not filed.
<!-- SECTION:FINAL_SUMMARY:END -->
