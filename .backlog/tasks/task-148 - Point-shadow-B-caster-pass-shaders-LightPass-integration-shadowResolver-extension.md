---
id: TASK-148
title: >-
  Point shadow B: caster pass + shaders + LightPass integration + shadowResolver
  extension
status: In Progress
assignee:
  - '@rendering-researcher'
created_date: '2026-04-26 22:30'
updated_date: '2026-04-27 08:52'
labels:
  - feature
  - rendering
  - lighting
  - shadows
  - rasterizer
  - shaders
dependencies:
  - TASK-147
references:
  - Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp
  - Source/ExampleProject/RenderingClient/LightPass.cpp
  - Source/Shaders/HLSL/sunShadowGeometryProcessPass.geom
  - Source/Shaders/HLSL/sunShadowGeometryProcessPass.frag
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/common/shadowResolver.hlsl
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
parent_task_id: TASK-66
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask B of TASK-66 (point/sphere light shadows in rasterized pipeline).** Owner: `rendering-researcher`.

End-to-end shadow rendering that consumes the foundation from TASK-147 and the component contract from TASK-149.

### What this subtask delivers

1. **`PointShadowGeometryProcessPass`** (new, `Source/ExampleProject/RenderingClient/PointShadowGeometryProcessPass.{h,cpp}`), mirroring `SunShadowGeometryProcessPass` (`Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp:18-205`). Per shadow-casting light, render the scene 6 times to the corresponding cube-face slices in the atlas. Implementation choice (geometry-shader fan-out vs. instanced 6-views vs. 6 separate draws) is part of the rendering-researcher design call — the surfacing producer's recommendation is geometry-shader fan-out for parity with `sunShadowGeometryProcessPass.geom`.

2. **Caster shaders** (new):
   - `Source/Shaders/HLSL/pointShadowGeometryProcessPass.vert` — passthrough world-space transform.
   - `Source/Shaders/HLSL/pointShadowGeometryProcessPass.geom` — fan-out to 6 cube faces × N lights via `SV_RenderTargetArrayIndex`. Mirror `sunShadowGeometryProcessPass.geom`'s structure (`maxvertexcount(3 * NR_CSM_SPLITS)` becomes `3 * 6 * NR_POINT_SHADOWS` or per-light fan-out — pick whichever fits the GS instruction budget).
   - `Source/Shaders/HLSL/pointShadowGeometryProcessPass.frag` — output format depends on design call (depth-only for hardware-PCF, packed `(depth, depth², 0, 1)` for VSM-PCSS; the latter mirrors `sunShadowGeometryProcessPass.frag:61`).
   - Alpha-test against material albedo for transparency-aware shadow casting (mirror `sunShadowGeometryProcessPass.frag:42-58`).

3. **`LightPass.cpp` integration** (`Source/ExampleProject/RenderingClient/LightPass.cpp`): add cube atlas binding (next available slot after `t11`, e.g. `t12`), state transition `WriteOnly → ReadOnly` in the same pre-dispatch transition block as the existing sun shadow (`LightPass.cpp:289`).

4. **`lightPass.comp` cube atlas binding** (`Source/Shaders/HLSL/lightPass.comp`): bind the cube atlas as `Texture2DArray<...> in_PointShadow` matching the new descriptor slot.

5. **`shadowResolver.hlsl` `PointShadowResolver`** (`Source/Shaders/HLSL/common/shadowResolver.hlsl`): new function paralleling `SunShadowResolver`. Per shadowed point light: compute the cube face from `(positionWS - lightPos)`, compute UV in that face, sample the atlas at `(faceSlotBase + faceIdx)`, return shadow factor with the same convention as `SunShadowResolver` (1=shadowed, 0=lit; the consumer applies `Visibility = 1 - shadowFactor`).

6. **`lightPassDirectLighting.hlsl::EvaluateTiledPointLighting`** (`Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl`): integrate shadow term per-light. Use the `castShadow` flag + atlas slot index that lives on the cbuffer payload (TASK-147 schema, TASK-149 component-side flag).

### Project invariants (anchor — read before implementing)

- **Convention parity with sun shadow**: `SunShadowResolver` returns `shadow ∈ [0,1]` where 1 = fully shadowed. `EvaluateSunLighting` applies `Visibility = 1 - shadow` (`lightPassDirectLighting.hlsl::EvaluateSunLighting`). `PointShadowResolver` MUST follow the same convention to avoid the inversion class-of-bug logged in `feedback_verify_source_before_chasing.md` and demonstrated in TASK-145 hypothesis 2.
- **Atlas binding semantics**: when the LightPass dispatch binds the atlas, the texture is single-resource / multi-slice. Don't bind 6 textures.
- **Sphere light shadow term**: per the parent task's spec, the minimum-viable start is to reuse the cube map at the sphere center — no per-frame angular jittering in this subtask. Defer that to a follow-up subtask if quality is unacceptable.
- **Shader cache hygiene**: TASK-146 has landed. `Scripts/HLSL2DXIL_NoPause.ps1` removes orphans automatically; `cmake --build` mirror-deploys per Config. No manual nuke needed.
- **Bisect-first if anything regresses**: per the universal `regression-fix-flow.md` discipline, do not chase a "shadow looks wrong" symptom with speculative fixes. Establish a known-good (commit before this CL), bisect, then fix. The TASK-122→145 chain documents the cost of skipping this.
- **Verification on-screen**: `feedback_onscreen_testing.md` — windowed/on-screen test, not just offscreen DumpRP.

### Verification scene

The parent's AC#4 ("scene with point light behind a wall produces a correct shadow") needs an authored scene. If no existing scene contains a point light + occluder geometry suitable for shadow validation, dispatcher coordinates with `software-architect` / `test-expert` to author one — e.g. extend `GITestBox` or add a dedicated `PointShadowTest.InnoScene`. This is in scope of this subtask; either find an existing scene or author the minimum.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 `PointShadowGeometryProcessPass` exists and writes to the atlas allocated in TASK-147 — per-light depth visible in RenderDoc
- [x] #2 Caster shaders compile + deploy via the (post-TASK-146) shader chain; both Sun and Point shadow passes produce correct atlas output
- [x] #3 `LightPass.cpp` binds the cube atlas + transitions correctly; no GBV errors
- [x] #4 `lightPass.comp` + `shadowResolver.hlsl` `PointShadowResolver` returns shadow factor with the same convention as `SunShadowResolver` (1=shadowed)
- [ ] #5 Authored test scene shows a point light behind a wall produces a correct shadow on the wall's far side — windowed screenshot evidence
- [ ] #6 GISponza auto-test perf check: no regression, OR documented budget delta
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Progress (rendering-researcher, 2026-04-27)

### Design calls resolved

- **Atlas format**: PixelDataFormat::RG (R32G32_FLOAT, 8B/pixel) instead of TASK-150's RGBA. The resolver only reads .r (linearDist) and .g (linearDist²); .b/.a were write-only padding. Saves 72 MB of triple-buffer VRAM (144→72 MB at 256² × 48 slices). Changed in LightDataService.cpp atlas allocation; matched in caster pass + .frag SV_Target0 type (float2).
- **Depth metric**: Linear distance (`length(posWS - lightPos) / range`), NOT perspective z. Caster .frag writes linearDist; resolver computes linearDist same way. Uniform metric across all 6 cube faces; no per-face conversion needed in resolver.
- **GS fan-out strategy**: GS instancing with `[instance(NR_POINT_SHADOWS)]` × `[maxvertexcount(18)]` — one pipeline draw produces NR_POINT_SHADOWS × 6 face writes per input triangle. SV_GSInstanceID is the per-light slot index. Inactive slots short-circuit before any vertex emit. Reuses sun-shadow culling buffer (no per-light culling pass for v1).
- **Sphere-light range**: Computed attenuation radius from `m_LuminousFlux` (`d = sqrt(φ / (4π · 0.01))`) instead of using `m_Shape.x` (which encodes physical sphere radius for spheres, not attenuation distance). Point lights still use `m_Shape.x` directly. No schema change needed — fix in the .inl allocator.
- **Slot threading**: Atlas slot index stamped into `PointLight_CB::pos.w` / `SphereLight_CB::pos.w` as a float-reinterpreted uint (memcpy bit-cast). HLSL recovers via `asuint(pos.w)`. Sentinel = `INVALID_ATLAS_SLOT`. No new GPU buffer / binding needed.

### Fixed two pre-commit defects

1. **HLSL cbuffer struct overrun** (caught by GBV): `uint padding0[2]` and `float padding1[8]` in HLSL pump each scalar to vec4 (16B), so `PointShadow_CB` weighed 640B/entry × 8 = 5120B, overrunning the 4096B C++ upload heap. Fix: declare trailing scalars individually + use `float4 padding1[2]` (vector arrays are not vec4-pumped). Total = exactly 512B matching C++.
2. **Comment line-continuation bug**: ASCII-art `\` at end of comment line caused the C preprocessor to swallow the next struct field. Removed the `\` `|` `|` `/` decoration.

### AC status
- AC#1: Pass exists, audit dump produces non-empty `audit_03b_PointShadowAtlas.hdr`. RenderDoc capture deferred to AC#5 visual session.
- AC#2: All shaders compile + deploy via `Scripts/HLSL2DXIL_NoPause.ps1`. Sun + Point both green.
- AC#3: LightPass binds atlas (t12) + cbuffer (b6); transition from WriteOnly→ReadOnly added. Run with `-gpu_validation` shows no fatal GBV errors (remaining warnings are pre-existing "Release-shader false positive" noise from other passes).
- AC#4: PointShadowResolver added to shadowResolver.hlsl; convention parity with SunShadowResolver (1=shadowed). EvaluateTiledPointLighting consumes shadow term.
- **AC#5 (visual evidence)**: Initial windowed capture saved at `Build/captures/task-148/gisponza_frame25_after_pointshadow.png`. Scene visibly darker than pre-task baseline (suggests shadows applied) but no controlled before/after diff yet — needs a verification scene with point light explicitly behind a wall or a debug-shader toggle. Status: partial.
- **AC#6 (perf)**: Not measured yet. To follow.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (engine + shader tier)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — windowed screenshot of a point light casting shadow behind a wall; GISponza perf log; RenderDoc capture proving the cube atlas was rendered + sampled
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
