---
id: TASK-148
title: 'Point shadow B: caster pass + shaders + LightPass integration + shadowResolver extension'
status: To Do
assignee: []
created_date: '2026-04-26 22:30'
updated_date: '2026-04-26 22:30'
labels:
  - feature
  - rendering
  - lighting
  - shadows
  - rasterizer
  - shaders
dependencies:
  - TASK-147
parent_task_id: TASK-66
priority: high
references:
  - Source/ExampleProject/RenderingClient/SunShadowGeometryProcessPass.cpp
  - Source/ExampleProject/RenderingClient/LightPass.cpp
  - Source/Shaders/HLSL/sunShadowGeometryProcessPass.geom
  - Source/Shaders/HLSL/sunShadowGeometryProcessPass.frag
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/common/shadowResolver.hlsl
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
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
- [ ] #1 `PointShadowGeometryProcessPass` exists and writes to the atlas allocated in TASK-147 — per-light depth visible in RenderDoc
- [ ] #2 Caster shaders compile + deploy via the (post-TASK-146) shader chain; both Sun and Point shadow passes produce correct atlas output
- [ ] #3 `LightPass.cpp` binds the cube atlas + transitions correctly; no GBV errors
- [ ] #4 `lightPass.comp` + `shadowResolver.hlsl` `PointShadowResolver` returns shadow factor with the same convention as `SunShadowResolver` (1=shadowed)
- [ ] #5 Authored test scene shows a point light behind a wall produces a correct shadow on the wall's far side — windowed screenshot evidence
- [ ] #6 GISponza auto-test perf check: no regression, OR documented budget delta
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**Sequencing**: BLOCKED on TASK-147 (foundation) AND TASK-149 (component flag). When both land, this subtask reads `lightCfg.castShadow` from the LightComponent and the atlas slot from `PointShadowConstantBuffer`.

**Co-author with software-architect / test-expert** for scene authoring (AC#5) if no existing scene has the required topology.

**Implementation-detail design call** (resolved by `rendering-researcher` as part of TASK-66's pre-implementation audit, NOT user-facing):

1. **Filter choice**: hardware-PCF on D32 atlas vs. moment-PCSS on packed-depth color atlas (parity with sun). Trade-off: PCF is cheaper per-sample, no AA penalty on cube seams; PCSS gives soft shadows but needs depth+depth² and the cube-seam handling is harder. Surfacing producer's MVP recommendation: hardware-PCF; defer cube-PCSS to follow-up. Final call by rendering-researcher.

2. **Atlas resolution**: 256² × 6 faces × 32 lights × D32 ≈ 12 MB per the surfacing producer's math. Confirm this matches the engine's VRAM headroom and the target visual quality.

Both calls land in TASK-147's design phase, then are reflected in this subtask's plan before implementation begins.
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
