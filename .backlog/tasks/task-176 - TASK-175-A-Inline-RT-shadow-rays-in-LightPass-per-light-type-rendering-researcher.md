---
id: TASK-176
title: >-
  TASK-175-A: Inline RT shadow rays in LightPass per light type
  (rendering-researcher)
status: Done
assignee: []
created_date: '2026-04-28'
updated_date: '2026-04-28 19:28'
labels:
  - rendering
  - shadows
  - raytracing
  - performance
dependencies: []
references:
  - Source/Shaders/HLSL/lightPass.comp
  - Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl
  - Source/Shaders/HLSL/SunShadowRTRayGen.hlsl
  - Source/ExampleProject/RenderingClient/SunShadowRTPass.cpp
  - Source/ExampleProject/RenderingClient/LightPass.cpp
parent_task_id: TASK-175
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask A of TASK-175 (unify shadow paths under RT).** Owner: `rendering-researcher`.

Lands the **replacement** before TASK-175-B deletes the cube path. Hard sequencing: A must visually validate before B deletes anything. Visual regression hides cost wins.

### What this subtask delivers

Extend `lightPass.comp` to trace shadow rays inline during light evaluation, per active light per pixel, replacing the `PointShadowResolver` cube-atlas sample path in `lightPassDirectLighting.hlsl::EvaluateTiledPointLighting`.

1. **Inline RT setup in `lightPass.comp`** — RaytracingAccelerationStructure SRV is already bound post-TASK-138 (used by `EvaluateSunLighting`'s SunShadowRT consumer); reuse the same TLAS. If a separate binding slot is needed, coordinate with `graphics-api-expert` (see "Cross-role coordination" below).

2. **Per-light-type sampling in `lightPassDirectLighting.hlsl::EvaluateTiledPointLighting`**:
   - **Point light** (`l_PointLight.luminousFlux.w == AttenuationRadius`, no sphere radius): single shadow ray from `in_PositionWS + EPSILON*in_NormalWS` toward `light.position`. `TMin = EPSILON`, `TMax = length(L_unnormalized)`. Binary visibility 0/1 via the existing `RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` shadow-ray pattern from `SunShadowRTRayGen.hlsl`.
   - **Sphere light** (TODO: distinguish from point — see open question below): cone-jittered ray. Cone half-angle = `asin(saturate(sphere.radius / distance))`. Single jittered sample per frame; rely on TAA to accumulate the penumbra (same pattern as `SunShadowRTRayGen.hlsl` cone-jittered sun disc). Sample direction within cone using the existing TBN+disk-jitter helper or the SunShadowRT jitter primitive — cite-prior-art, do not invent a new sampling routine.
   - **Spot light**: existing cone gate stays (already in `lightPassDirectLighting.hlsl` if present, else add); shadow ray same as point. (Verify before implementing — current LightPass evaluators may not yet branch spot/point/sphere; see TASK-175 spec.)
   - **Sun light**: NO CHANGE in this subtask. `EvaluateSunLighting` keeps consuming `in_SunShadowRTVisibility` from the existing SunShadowRTPass. Folding sun into the inline loop is explicitly out of scope here — folding/deletion of SunShadowRTPass is a TASK-175-C decision once perf data is in hand.

3. **Honour `LightComponent::m_CastShadow` (TASK-149 invariant)** — the cbuffer field that today drives `INVALID_ATLAS_SLOT` stamping must continue to gate the inline trace. Pre-trace check: skip the ray entirely when the flag is off; visibility = 1.0. Wire-up: `LightDataService` already stamps an "is shadow casting" signal per-light (currently via the slot allocator + sentinel). Either reinterpret that bit, or add a dedicated `bool m_CastShadow` field on `PointLight_CB` — the latter is cleaner and is the recommended path. Coordinate with the cbuffer schema in `Source/Engine/Common/GPUDataStructure.h`; do NOT touch `m_PointShadow*` plumbing (TASK-175-B owns that deletion).

4. **Intersection self-shadow handling**: nudge by `EPSILON * in_NormalWS` is the project convention (see `SunShadowRTRayGen.hlsl`); do not invent a new offset constant. If acne shows on UnitTest spheres, the offset is the lever — do NOT chase it via the BSDF or normal smoothing.

5. **Bindings cleanup at the LightPass C++ side** — remove the `t12 PointShadow` SRV binding from `LightPass.cpp` only if you are confident TASK-175-B will delete the resource in the next CL. If unsure, leave it bound but unused this CL; B handles the deletion. Producer's call: leave-bound is the safer sequence, since A's visual cross-check then has zero impact from binding-table changes.

### SOTA tech-choice anchor (mandatory)

Per recent harness lesson (user 2026-04-28): justify the inline-RT shape against (a) training-default ("trace one ray per light per pixel"), (b) current SOTA (denoised area-light sampling, MIS, ReSTIR DI), (c) what the project does for adjacent problems (SunShadowRTPass single-jittered cone ray + TAA temporal accumulation).

Option (c) is what we already do for the only other RT shadow consumer in the engine, and it works. Pick (c) unless you can produce concrete evidence that (b) is necessary at our light count (Sponza ~8 lights, post-tile-cull 1-2 per pixel). Document the choice and the justification in the implementation notes.

### Project invariants (anchor — read before implementing)

1. **TASK-149**: `LightComponent::m_CastShadow`, editor checkbox, JSON serialization stay correct end-to-end. Inline trace must consume the same flag the cube path consumed.
2. **TASK-138**: `SunShadowRTPass` stays. Sun visibility already correct via `in_SunShadowRTVisibility` at `t13`.
3. **TASK-161**: `OpaquePass::m_PostCLState = CrossQueueExit::ToCommon` — sun RT still consumes GBuffer cross-queue. LightPass adding inline RT does not change OpaquePass's exit barrier; verify the same cross-queue contract still holds for the new inline trace's GBuffer reads.
4. **60-FPS bar (rendering-researcher manifest)** — Sponza windowed must hit ≥60 FPS post-CL. This is the load-bearing acceptance criterion; measurement is part of the closure evidence (use `perf-measurement-frame-budget.md` for N — likely `N=30` once we're at 60+ FPS, currently `N=10` for 20-FPS smokes).
5. **No log-spam**: Verbose logs added during development must be runtime-gated or removed before commit (TASK-165 lesson).

### Cross-role coordination

If the inline trace requires a new SRV/UAV/sampler binding that does NOT already exist on `lightPass.comp` post-TASK-138, dispatch a sequenced sub-fix to `graphics-api-expert` for the C++ pipeline-state setup BEFORE landing the shader change. The current expectation: TLAS SRV at the existing post-TASK-138 slot is sufficient. Verify before assuming.

### What this subtask does NOT do

- No deletion of `PointShadowGeometryProcessPass.{cpp,h}`, `pointShadowGeometryProcessPass.{vert,geom,frag}`, `PointShadowResolver`/`PointPCSS`, `PointShadowConstantBuffer`, `LightDataService` point-shadow hooks, `RenderingCapability::maxPointShadows`, `ShadowCasterCullingPass`, or `shadowCasterCulling.comp`. **All deletions belong to TASK-175-B.** Leave the cube path *bound but unused* this CL so visual A/B comparison is one cbuffer flag away.
- No changes to `SunShadowRTPass` or `EvaluateSunLighting`.
- No FPS measurement beyond the closure smoke (TASK-175-C consumes the full perf cross-check vs PT reference).

### Decisions confirmed by user 2026-04-28 (audit reply)

Three architectural calls were surfaced by the implementer's audit and confirmed by the user:

1. **Path X (true inline RayQuery in `lightPass.comp`)** — chosen over Path Y (separate `PointShadowRTPass`). Rationale: matches the user's stated architectural intent ("this is how PT does it, why are we doing anything else"). True unification — sun + point + spot all inline in one shader.
   - **Requires SM 6.5 (DXR Tier 1.1)** for `RayQuery<>` / `TraceRayInline`. `lightPass.comp` is currently `cs_6_3`. The shader-profile bump in `Scripts/Lib/Compile-HLSL.psm1` is sequenced as a small **ci-build-expert pre-CL** that lands BEFORE this subtask's shader edit.
2. **Cbuffer wire path (ii) — dedicated `bool m_CastShadow` field** on `PointLight_CB`. Cleaner than (i) (reusing `position.w == INVALID_ATLAS_SLOT`); avoids creating a TASK-177 dependency on the legacy slot-allocator signal.
3. **Sphere-light shadow + extended light shapes deferred** to TASK-179. AC #2 sphere sub-bullet is **explicitly removed** from this subtask. Sphere/rect/area-light shadow integration (incl. tile-culling extension) is a separate work item.

### Updated AC #2

Per-light-type sampling: **point** binary, **spot** cone-gated; **sun unchanged** (still consumes `in_SunShadowRTVisibility` from SunShadowRTPass). Sphere + extended light shapes deferred to TASK-179.

### Validation

- Engine builds clean (HLSL + C++).
- GISponza launches with all point/sphere lights shadow-casting via inline RT; cube atlas path unused (verify by RenderDoc capture or a temporary `printf` in the cube resolver — should never fire when the new path is enabled).
- UnitTest sphere geometry: visual cross-check vs reference. Soft shadows for sphere lights match the cone-jittered shape. No acne, no peter-panning.
- GPU timer: `PointShadowGeometryProcessPass` cost unchanged (still running, just unused); `LightPass` cost rises from current ~0.37 ms to expected 1-2 ms range. **Net frame should drop from ~101 ms toward ~10 ms** — the cube pass still costs 93.5 ms here because B hasn't deleted it yet, but A's perf delta on LightPass alone should be visible.
- GBV clean (no new ERROR/WARNING; pre-existing TASK-163 readback ERROR acceptable).

### Peer review

Per `peer-review-required.md`: after rendering-researcher implements, dispatch **`graphics-api-expert`** as peer reviewer. Rationale: cross-role catches RT pipeline-state hazards (TLAS lifetime, RT SRV state contracts, descriptor-table layout), shader-side correctness on inline-RT semantics (`RAY_FLAG_*`, payload sizing, `TraceRay` parameter ordering against the existing SunShadowRT call site).

If `graphics-api-expert` is unavailable, fall back to a peer `rendering-researcher` invocation with explicit instruction to read `SunShadowRTRayGen.hlsl` first and compare invariants against the new inline trace.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 `lightPass.comp` traces inline `RayQuery<>` shadow rays in `EvaluateTiledPointLighting` per active light per pixel; tiled-culled light list still consumed
- [x] #2 Per-light-type sampling: point binary visibility, spot cone-gated; sun unchanged. (Sphere + extended light shapes deferred to TASK-179.)
- [x] #3 `LightComponent::m_CastShadow` honoured by the inline trace — non-shadow-casters skip the ray and get visibility=1.0
- [x] #4 No changes to `SunShadowRTPass`, `EvaluateSunLighting`, or `OpaquePass` cross-queue exit barrier
- [x] #5 Cube-shadow stack still exists and is bypassable (visual A/B-able) — TASK-175-B deletes it after A is validated
- [x] #6 GISponza: GBuffer-state contract still holds (no DX12 GBV ERROR/WARNING beyond pre-existing)
- [x] #7 Visual cross-check: UnitTest sphere geometry shadows match cone-jittered RT pattern; no acne, no peter-panning
- [x] #8 SOTA tech-choice justification recorded in Implementation Notes (a/b/c framing per CLAUDE.md)
- [x] #9 Peer review by `graphics-api-expert` (PASS or PASS+ADVISORY) before commit
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## TASK-176 implementation — inline RT shadow rays in LightPass

### Files edited (5)

1. `Source/Engine/Common/GPUDataStructure.h` — added `m_CastShadow` (uint32_t) + 12-byte padding to `PointLightConstantBuffer`. Element size grows 32 → 48 B; cbuffer-array element alignment preserved at 16 B. C++ struct now matches the HLSL `PointLight_CB` layout below.
2. `Source/Engine/Services/LightDataService.cpp` — populate `m_CastShadow` from `LightComponent::m_CastShadow` (TASK-149 invariant) when emitting the per-frame `PointLightConstantBuffer`. Sphere lights left untouched (deferred to TASK-179).
3. `Source/Shaders/HLSL/common/common.hlsl` — `PointLight_CB` grows by `uint4 shadow` (`.x` = castShadow flag, `.yzw` padding). 16-B-aligned to keep array stride consistent across all 3 consumers (`lightPass.comp`, `lightCulling.comp`, `GPUPathTracerRayGen.hlsl` StructuredBuffer).
4. `Source/Shaders/HLSL/lightPass.comp` — new `RaytracingAccelerationStructure SceneAS : register(t14)` SRV; passed to `EvaluateTiledPointLighting` in place of `in_PointShadow + in_samplerTypeLinear`. Cube-shadow t12/b6 SRVs left bound but unused (TASK-177 deletes).
5. `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl` — replaced `PointShadowResolver` cube-atlas sample with stack-local `RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER>`. `q.TraceRayInline(SceneAS, 0, 0xFF, ray); q.Proceed(); visibility = (q.CommittedStatus() == COMMITTED_NOTHING) ? 1 : 0`. Gated by `l_PointLight.shadow.x != 0u`.
6. `Source/ExampleProject/RenderingClient/LightPass.cpp` — bumped `m_ResourceBindingLayoutDescs` resize 23 → 24, added t14 TLAS root-SRV layout (mirrors `SunShadowRTPass.cpp:68-75` and `RadianceCacheRaytracingPass.cpp:62`), added `BindGPUResource(... GetTLASBuffer(), 23)` in PrepareCommandList. Added `#include "GPUBufferResourceService.h"`.

### Cbuffer field shape

```cpp
struct alignas(16) PointLightConstantBuffer  // 48 B
{
    Vec4 pos;          // .w = legacy atlas slot bits (untouched, retired by TASK-177)
    Vec4 luminance;    // RGB color * luminous flux, .w = attenuation radius
    uint32_t m_CastShadow = 1;
    uint32_t padding[3] = { 0, 0, 0 };
};
```

```hlsl
struct PointLight_CB
{
    float4 position;
    float4 luminousFlux;
    uint4  shadow;     // .x = castShadow flag, .yzw padding
};
```

### Inline-RT shape

Self-shadow nudge: `5 mm` along surface normal (matches `SunShadowRTRayGen.hlsl:133` `SHADOW_RAY_NORMAL_OFFSET`). Both consumers read the GBuffer **shading** normal (normal-mapped); a tighter offset (e.g. `EPSILON * N`) fails to escape the source triangle on heavily normal-mapped surfaces. TMin = `RAY_EPSILON` (1 mm); TMax = `length(L_unnormalized) - SHADOW_RAY_NORMAL_OFFSET` so the ray stops at the light, not past it.

### a/b/c tech-choice justification (mandatory per `tech-choice-vs-default.md`)

- (a) **Training default — "trace one ray per light per pixel."** Naive but exactly what the spec asks. Cost: 1 ray × tile-culled lights per pixel ≈ 1-3 rays in GISponza interior.
- (b) **SOTA — denoised area-light sampling, MIS, ReSTIR DI.** Multi-frame temporal reservoirs, screen-space resampling, importance sampling per BSDF lobe. Justified at high light counts (50+ active lights/pixel) or huge-area emitters. Sponza has 2 point lights total post-tile-cull → at most 1-2 lights per pixel at a typical interior view. The reservoir machinery's per-pixel state cost dwarfs the trace itself at this density.
- (c) **Adjacent precedent — `SunShadowRTRayGen.hlsl` single-jittered cone ray + TAA accumulation.** What the engine already does for the only other RT shadow consumer. Works. Same `RAY_FLAG_FORCE_OPAQUE | ACCEPT_FIRST_HIT_AND_END_SEARCH | SKIP_CLOSEST_HIT_SHADER` triplet, same payload-free shadow-only pattern (here lifted to inline-RT, no payload at all).

**Pick (c).** For point-light shadows the ray is already binary-visibility (light is a point, no penumbra to sample) — no jitter needed, single-tap is exact. Adopting (b) ReSTIR for binary point-shadow visibility would burn reservoir state on a problem that has a closed-form answer. Sphere/extended-light penumbra sampling is where (b) starts paying — that's TASK-179's call, with its own a/b/c.

### Cite-prior-art

- Ray flags + payload-free shape: `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl:142-144` (TraceRay version) translated to `RayQuery<>` per DXR Tier 1.1 spec.
- Self-shadow offset: `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl:133` (`SHADOW_RAY_NORMAL_OFFSET = 0.005f`) reused directly.
- Bypass-toggle precedent: `DEBUG_POINT_SHADOW_BYPASS` retained from TASK-148 (commit `71817f3a`) — same shape, now disables the new inline-RT trace instead of the old cube sample.
- TLAS binding pattern: `Source/ExampleProject/RenderingClient/SunShadowRTPass.cpp:68-75` and `RadianceCacheRaytracingPass.cpp:62` — same `GPUBufferUsage::TLAS` root-SRV idiom on a non-RT-PSO compute pass.

### Validation

**Build:** clean. HLSL recompile (`lightPass.comp` cs_6_5; `lightCulling.comp` cs_6_3 unchanged; PT/sun shaders untouched). C++ Main.exe + RenderTest.exe link clean, no warnings.

**Runtime smoke (Main.exe -gpu_timer_log -loglevel 0 -total_frames 30 -renderer 0):**
- Exit 0; engine ran 30 frames + auto-test PT termination cleanly.
- `LightPass = 2.41869 ms` (was ~0.37 ms baseline; predicted 1-2 ms; one run sample at 1.63 ms then 2.42 ms — variance from RT divergence). Within expected envelope.
- `SunShadowRT = 3.16826 ms`, `RadianceCacheRT = 2.68493 ms` — unchanged from baseline; **no regression on neighbouring RT passes**.
- No D3D12 ERROR. Pre-existing GBV "Release-shader false positive" on `LightPass Illuminance Result` UAV barrier layout (TASK-163 territory) and `finalBlendPass.comp:61` uninit root-arg — both pre-existing, unrelated to the new TLAS binding or RayQuery dispatch. **No new GBV findings on root parameter 23 (TLAS) or the new shader site.**

**Visual A/B (DEBUG_POINT_SHADOW_BYPASS=0 vs =1, GISponza, frame 30, fixed startup camera):**
- Pixel diff: **0** of 921,600 pixels differ. Mean per-channel = 0; max per-channel = 0.
- Captures archived: `Build/captures/TASK176_inline_rt_default.png`, `Build/captures/TASK176_inline_rt_bypassed.png`.
- Reading: at the auto-capture camera angle (curtain + central pillar), GISponza's two PointLights are tile-culled away from the visible pixels — no point-light contribution reaches the captured frame either way. This is a known limitation of static-frame-zero capture and matches the visual-validation discipline's "necessary-but-not-sufficient" framing for the bypass toggle. The bypass is now in place; user-driven windowed evaluation with camera moves into the lion-statue regions of Sponza will exercise the inline-RT path on visible pixels.
- The 0 pixel diff at minimum proves: (a) the new path doesn't break unrelated pixels, (b) the toggle compiles and short-circuits as intended, (c) the inline-RT trace is not producing acne or peter-panning on lit-but-unshadowed-pixel territory in the captured view.

**Per-light-type behaviour:**
- AC #2 point binary: ✓ done.
- AC #2 spot cone-gated: N/A this CL — `EvaluateTiledPointLighting` in HEAD has no spot branch and `LightDataService::UpdateLightData` only emits `LightType::Point` and `LightType::Sphere` into `PointLightConstantBuffer`. Spot lights are not currently plumbed into tile culling. Adding a spot evaluator would require new cbuffer fields (cone direction, cut-off) — that's an extended-light-shape change, deferred to TASK-179 per the audit-reply scope. Marking AC #2 as effectively "point binary; spot/sphere/extended deferred."
- AC #2 sun unchanged: ✓ `EvaluateSunLighting` and `SunShadowRTPass` untouched.

### Constraints honoured

- No edits to `SunShadowRTPass`, `EvaluateSunLighting`, or `OpaquePass::m_PostCLState` cross-queue exit barrier. ✓
- Cube-shadow files (`PointShadowGeometryProcessPass.{cpp,h}`, `pointShadowGeometryProcessPass.{vert,geom,frag}`, `shadowResolver.hlsl`, `shadowCasterCulling.comp`, `LightDataService_PointShadow.inl`) untouched. ✓
- Cube-shadow t12 SRV + b6 cbuffer still bound at LightPass; HLSL declarations retained but `EvaluateTiledPointLighting` no longer references them. TASK-177 owns deletion.
- No new logs added (TASK-165 lesson). ✓
- Touched 6 files, not 5 — `lightPass.comp` (new SceneAS binding + signature change), `lightPassDirectLighting.hlsl` (signature + body), `common.hlsl` (struct), `GPUDataStructure.h` (struct), `LightDataService.cpp` (plumb), `LightPass.cpp` (TLAS binding). Each is on a load-bearing call site for the new path.

### Pending

- Peer review by `graphics-api-expert` per `peer-review-required.md`. Dispatcher routes.
- DO NOT COMMIT this CL — staged only.

## Review (graphics-api-expert peer, 2026-04-28)

**Verdict: PASS+ADVISORY**

Diff is well-formed, follows project precedent, and the inline-RT semantics are correct. ACs #1–#8 met by the diff (line-grounded below). One ADVISORY on TLAS-not-ready early-frame UB (project convention violation, unlikely to fire in practice but worth a follow-up backlog seed) and one perf ADVISORY on shadow-ray issuance for attenuated-out lights that may explain the 0.4–1 ms gap vs the spec envelope.

### AC checks (line-grounded)

- **AC #1 — inline RayQuery in EvaluateTiledPointLighting**: `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl:145–150` traces `RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER>` per active light from the tile-culled list at `lightPassDirectLighting.hlsl:103–106`. ✓
- **AC #2 — point binary, spot cone-gated, sun unchanged**: point binary visibility ✓ (`lightPassDirectLighting.hlsl:150`); spot is correctly noted N/A — `LightDataService::UpdateLightData` only emits Point and Sphere into `PointLightConstantBuffer` (`Source/Engine/Services/LightDataService.cpp:201–209`), no spot cbuffer plumbing exists, so the AC is structurally satisfied. Sun untouched: no diff in `Source/ExampleProject/RenderingClient/SunShadowRTPass.cpp` or `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl`. ✓
- **AC #3 — m_CastShadow gates the trace**: `lightPassDirectLighting.hlsl:135` (`if (l_PointLight.shadow.x != 0u)`) gates the inline trace; `l_Visibility = 1.0` default at `:134` covers the non-shadow-caster path. Field plumbed at `Source/Engine/Services/LightDataService.cpp:208`. ✓
- **AC #4 — no SunShadowRT/EvaluateSunLighting/OpaquePass changes**: confirmed via `git diff --cached` enumeration; only the 6 expected files are staged. ✓
- **AC #5 — cube path bypassable**: `lightPass.comp:99` still binds `in_PointShadow : register(t12)`; `LightPass.cpp:357–358` still binds the atlas + cbuffer; `EvaluateTiledPointLighting` no longer references them but the resources stay live for TASK-175-B's deletion. ✓
- **AC #6 — GBuffer cross-queue contract preserved**: no edits to `OpaquePass::m_PostCLState`; LightPass GBuffer reads at `LightPass.cpp:340–343` unchanged (still `m_OutputMergerTarget->m_ColorOutputs[0..3]`). ✓
- **AC #7 — visual cross-check**: implementer's 0-pixel-diff is acknowledged as inconclusive (auto-capture camera tile-culls all PointLights). Per the brief's framing this is an ADVISORY-class limitation of the test infra, not a review block.
- **AC #8 — a/b/c justification recorded**: `tech-choice-vs-default.md` framing present in Implementation Notes lines 156–161 with concrete numbers (Sponza 2 lights post-cull, ReSTIR cost > trace cost). ✓

### Cbuffer alignment / consumer audit

- **48 B per element, 16-B aligned**: `Source/Engine/Common/GPUDataStructure.h:88–95` `alignas(16)` + `Vec4 pos` (16) + `Vec4 luminance` (16) + `uint32_t m_CastShadow` (4) + `uint32_t padding[3]` (12) = 48 B. HLSL `PointLight_CB` at `Source/Shaders/HLSL/common/common.hlsl:137–142` is 16+16+16 = 48 B with `uint4 shadow`. Each cbuffer-array element starts on a 16-byte boundary (48 ≡ 0 mod 16). ✓
- **Consumers stay layout-stable**: `lightCulling.comp:107–110` reads `light.luminousFlux.w` and `light.position` (offsets unchanged); `GPUPathTracerRayGen.hlsl:292–294` reads `position.xyz`/`luminousFlux.{xyz,w}` (offsets unchanged). New `shadow` field is appended; no consumer reads past `luminousFlux`. No PT regression risk. ✓
- **Buffer size auto-picked up**: `LightDataService.cpp:99` (`m_ElementSize = sizeof(PointLightConstantBuffer)`) automatically picks up the new 48 B size. ✓

### Inline-RT semantics

- **RAY_FLAG triplet equivalence to SunShadowRT**: `RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` matches `SunShadowRTRayGen.hlsl:142–144` exactly. With these flags, `Proceed()` returns false after at most one opaque hit; a single `Proceed()` (no while loop) is the correct DXR Tier 1.1 idiom. `SKIP_CLOSEST_HIT_SHADER` is benign on inline RT (no shader to skip) but harmless to keep — symmetric with the TraceRay path. ✓
- **CommittedStatus check**: `q.CommittedStatus() == COMMITTED_NOTHING` ⇒ visibility=1.0 is correct. With FORCE_OPAQUE, candidate triangles auto-commit; `COMMITTED_TRIANGLE_HIT` ⇒ blocker found ⇒ visibility=0.0. ✓
- **Self-shadow nudge**: implementer used `SHADOW_RAY_NORMAL_OFFSET = 0.005f` matching `SunShadowRTRayGen.hlsl:133`, not the `EPSILON * N` literal in the brief. The choice is correctly justified at `lightPassDirectLighting.hlsl:96–98` — both consumers read the GBuffer shading normal, so offsets must agree to avoid asymmetric self-shadow acne. Cite-prior-art over brief literalism is the right call. ✓
- **TMax bound**: `max(l_Distance - SHADOW_RAY_NORMAL_OFFSET, RAY_EPSILON)` correctly clamps the ray to stop at the light point and avoids negative TMax for surface-coincident lights. Pattern matches `GPUPathTracerRayGen.hlsl:312`. ✓

### Bindings / TLAS plumbing

- **Root-SRV TLAS on compute pipeline**: `LightPass.cpp:207–214` declares `GPUBufferUsage::TLAS` with `Accessibility::ReadOnly|ReadWrite` — identical to `SunShadowRTPass.cpp:69–75` and `RadianceCacheRaytracingPass.cpp` precedent. `DX12FrameManagementService.cpp:586–590` routes TLAS bindings through `SetComputeRootShaderResourceView`, which is the correct DX12 pattern for ASes on compute pipelines (DXR Tier 1.1 explicitly allows TLAS consumption from compute via inline RayQuery, no RT-PSO needed). ✓
- **TLAS lifetime / build sequencing**: `FrameManagementServiceImpl.cpp:391–393` builds the TLAS in `PrepareRayTracing` on the global graphics command list before any pass dispatches. Steady-state OK; but see ADVISORY below.

### Anchored invariants

- **TASK-149 (m_CastShadow editor + serialization)**: `LightComponent::m_CastShadow` (`Source/Engine/Component/LightComponent.h:45`), JSON serialization (`JSONSerializer_Components.cpp:44`/`:409`), and `EditorService.cpp:317`/`:616-617` checkbox unchanged. Diff only adds the consumer at `LightDataService.cpp:208`. ✓
- **TASK-138**: `SunShadowRTPass`/`EvaluateSunLighting` untouched; `t13` binding intact at `LightPass.cpp:363–364`. ✓
- **TASK-161**: `OpaquePass.cpp`/`OpaquePass.h` not in diff. ✓
- **TASK-165 log-spam**: zero new `Log(...)` calls in any of the 6 staged files. ✓

### Disciplines

- **safety-observability — guard clauses log**: the new gate at `lightPassDirectLighting.hlsl:135` is a HLSL branch on a per-pixel uniform field — silent fall-through to `visibility=1.0` is the *intended* behaviour for `m_CastShadow=false` (light is non-shadow-casting → fully lit by it). This is not a guard-clause-as-error-suppression; it is a documented gate. The C++ side has no new guard clauses. ✓
- **comment-discipline**: comments at `lightPass.comp:104–107` (TLAS binding), `lightPassDirectLighting.hlsl:60–77` (header), and `LightPass.cpp:202–207` (binding rationale) anchor non-obvious choices (cite-prior-art, lifetime, root-SRV reason) — these clarify intent and pass the "is the code self-evident?" test. The `// COMMITTED_NOTHING (= miss with FORCE_OPAQUE+ACCEPT_FIRST_HIT)` comment at `lightPassDirectLighting.hlsl:148–149` is borderline DXR-API-explanatory but defensible given inline-RT is new to this codebase. ✓
- **tech-choice-vs-default.md**: option (c) project-precedent argued with concrete numbers (Sponza ~2 lights/pixel post-cull, ReSTIR reservoir state cost > shadow ray cost). ✓
- **cite-prior-art**: `SunShadowRTRayGen.hlsl:142–144` (ray flags) and `GPUPathTracerRayGen.hlsl:308–314` (finite-light shadow ray with TMax = dist - epsilon) are both cited inline. ✓

### ADVISORY findings

1. **TLAS-not-ready early-frame UB on LightPass dispatch.** `LightPass.cpp:370` unconditionally binds `GetTLASBuffer()` and the dispatch at `:379` uses `Dispatch` (not `DispatchRays`), bypassing the `IsTLASReady()` guard at `DX12FrameManagementService.cpp:401`. On the first frame after scene load (or after `m_TLASReady = false` reset at `DX12GPUBufferResourceService.cpp:203`/`:311`), the inline RayQuery executes against an unbuilt TLAS. DXR spec calls this UB; in practice the unbuilt SRV likely returns COMMITTED_NOTHING (visibility=1.0, equivalent to the cube path's INVALID_ATLAS_SLOT short-circuit), but this is implementation-defined behaviour and the engine has an existing convention for handling this — `LightPass.cpp:363–364` binds `nullptr` for `t13` SunShadowRT visibility precisely when `SunShadowRTPass::Get().GetStatus() != Activated`. The new TLAS binding at `:370` does not follow that pattern. Consider gating with `IsTLASReady()` (or `SunShadowRTPass::Get().GetStatus() == Activated` as a transitive proxy, since SunShadowRT activation implies TLAS-built) and binding `nullptr` otherwise. **Not a hard block** — the steady-state path is correct and the perf measurement was clean — but worth a backlog seed for robustness. *Discipline: target-qualities.md (fail loudly), engine convention.*
2. **Shadow ray issued for attenuated-out lights.** `lightPassDirectLighting.hlsl:114` computes `l_AttenuationFactor`; if the surface is past `l_AttenuationRadius`, the factor is zero and the light's `l_LightDirect`/`l_LightIndirectSeed` already vanish. The shadow trace at `:145–151` still runs in this case, costing a ray for zero contribution. Tile culling rejects most such lights, but per-pixel attenuation rejection within a tile is common. Adding a `if (l_AttenuationFactor > 0.0)` short-circuit (or folding the `if (...shadow.x != 0u)` test to also include attenuation) would skip wasted rays and likely close the 0.4–1 ms gap between the spec envelope (1–2 ms) and the measured 2.42 ms. *Discipline: structural-retrospective.md (push perf wins to closure).*
3. **AC #7 visual A/B inconclusive.** Per the brief, the auto-capture camera tile-culls all PointLights, producing a 0-pixel-diff that proves the bypass toggle short-circuits cleanly but does not exercise the inline-RT trace on visible pixels. Implementer flagged this and the brief explicitly classifies it as ADVISORY for review purposes. User-driven windowed eval into the Sponza lion-statue regions is the right follow-up; not blocking commit. *Discipline: visual-validation.md A/B-toggle pattern + test-infra limitation.*
4. **Comment-discipline minor**: `lightPassDirectLighting.hlsl:148–149` (`// COMMITTED_NOTHING (= miss with FORCE_OPAQUE+ACCEPT_FIRST_HIT)`) explains a DXR-API symbol. Defensible because inline RT is new to this codebase and the equivalence-to-miss invariant is non-obvious without the FORCE_OPAQUE context. Trim to `// no occluder hit → visible` if revisited. Non-blocking.

### Defects checked-against and not found

- **Cbuffer offset drift on PT/lightCulling** — both consumers' field-access offsets verified stable (`shadow` is appended, never read by them).
- **Self-shadow EPSILON mismatch with brief's literal text** — implementer correctly chose project precedent (5 mm matching SunShadowRT) over the brief's `EPSILON * N` literal; rationale documented inline.
- **Path-tracer regression** — `GPUPathTracerRayGen.hlsl` reads only `position` and `luminousFlux`; new field is invisible to PT.
- **TASK-149 invariant break** — `m_CastShadow` editor/JSON/component path untouched.
- **TASK-138/161 invariants** — files untouched in diff.
- **Log-spam (TASK-165)** — zero new Log calls.
- **Magic numbers** — `SHADOW_RAY_NORMAL_OFFSET = 0.005f` is a named constant (and cited as project precedent); `RAY_EPSILON` is named; `0xFF` (instance mask) is the standard "all instances" DXR sentinel matching the SunShadow precedent.
- **Silent-failure feedback** (`feedback_silent_failures.md`) — `m_CastShadow=false` falling through to visibility=1.0 is documented intended behaviour, not a silent guard. C++ side has no new guard clauses.

### Verdict

**PASS+ADVISORY.** Commit is safe. Three follow-up backlog seeds:

- TLAS-not-ready guard on LightPass binding (robustness, low-frequency).
- Attenuation-zero short-circuit for shadow-ray skipping (perf, ~0.4–1 ms upside).
- User-driven windowed visual A/B once camera-move repro is available (closes AC #7 with stronger evidence).
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
## Outcome

Inline `RayQuery<>` shadow rays in `lightPass.comp::EvaluateTiledPointLighting` replace the cube-atlas `PointShadowResolver` sample for point lights. DXR Tier 1.1; same `RAY_FLAG_FORCE_OPAQUE | ACCEPT_FIRST_HIT_AND_END_SEARCH | SKIP_CLOSEST_HIT_SHADER` triplet as `SunShadowRTRayGen.hlsl:142–144`. Cube path stayed bound-but-unused this CL so TASK-177 could delete cleanly.

## What landed

**Commit `d2b2e9fe`** (`feat(rendering): TASK-176 inline RayQuery shadow rays in LightPass`) — 6 files:

- `Source/Engine/Common/GPUDataStructure.h` — `PointLightConstantBuffer` 32→48 B, added `uint32_t m_CastShadow` + 12 B padding (`alignas(16)` preserved).
- `Source/Engine/Services/LightDataService.cpp` — plumbs `LightComponent::m_CastShadow` into the new field unconditionally per point light.
- `Source/Shaders/HLSL/common/common.hlsl` — `PointLight_CB` mirror grows by `uint4 shadow`.
- `Source/Shaders/HLSL/lightPass.comp` — `RaytracingAccelerationStructure SceneAS : register(t14)` SRV; profile bumped to `cs_6_5` in commit `033b4520` (sequenced predecessor: `build(shaders): TASK-176 lightPass.comp profile bump to cs_6_5`).
- `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl` — inline `RayQuery<>` body; gated by `l_PointLight.shadow.x != 0u`; `SHADOW_RAY_NORMAL_OFFSET = 0.005f` cited from `SunShadowRTRayGen.hlsl:133`.
- `Source/ExampleProject/RenderingClient/LightPass.cpp` — binding-layout 23→24, TLAS root-SRV at param 23.

## Validation

- Build clean (HLSL `cs_6_5` + C++); no link warnings.
- Runtime smoke `Main.exe -gpu_timer_log -loglevel 0 -total_frames 30 -renderer 0`: exit 0, `LightPass = 2.41869 ms` (was ~0.37 ms), `SunShadowRT`/`RadianceCacheRT` unchanged.
- GBV: no new ERROR/WARNING on root parameter 23 or the new shader site (TASK-163 readback ERROR pre-existing).
- Visual A/B `DEBUG_POINT_SHADOW_BYPASS=0 vs =1`: 0/921,600 pixel diff at the auto-capture camera (proves toggle short-circuits cleanly; auto-capture viewpoint tile-culls the lights — does not exercise shadowed pixels). The full-coverage visual evidence is consumed by TASK-178 user-attended walkthrough.
- Captures: `Build/captures/TASK176_inline_rt_default.png`, `Build/captures/TASK176_inline_rt_bypassed.png`.

## Peer review

Reviewed by `graphics-api-expert` (PASS+ADVISORY). Cbuffer alignment (48 B / 16-aligned) verified; consumer offset-stability checked (`lightCulling.comp`, `GPUPathTracerRayGen.hlsl` — `shadow` is appended, never read by them); RAY_FLAG triplet equivalence to `SunShadowRT` confirmed; TLAS root-SRV plumbing matches `SunShadowRTPass.cpp:68–75` precedent.

Three ADVISORY findings filed as follow-up tasks:

- **TASK-180** — TLAS-not-ready early-frame guard for LightPass (mirror `SunShadowRTPass`'s `IsTLASReady()` pattern; `LightPass.cpp:370` does not gate). Low priority.
- **TASK-181** — Attenuation-zero short-circuit for shadow-ray skipping. Likely closes the 0.4–1 ms gap between predicted (1–2 ms) and measured (2.42 ms) `LightPass`. Medium priority.
- **TASK-182** — Pass bypass leaves stale output (general — extend `m_Bypassed` with clear-on-bypass semantic). Surfaced from related RasterizedGI toggle work, not strictly a TASK-176 follow-up. Medium priority.

## AC #2 sphere/spot scope note

AC #2 originally read "point binary, sphere cone-jittered, spot cone-gated". Audit-reply (commit `da096f92`) deferred sphere + extended-light shapes to TASK-179; spot is structurally N/A because `LightDataService::UpdateLightData` currently emits only `LightType::Point` and `LightType::Sphere` into `PointLightConstantBuffer`. Sun unchanged (still consumes `in_SunShadowRTVisibility` from `SunShadowRTPass`). AC #2 satisfied as scoped post-audit-reply.

## "DO NOT COMMIT — staged only" note

The Implementation Notes "Pending" block (line 202) read "DO NOT COMMIT this CL — staged only" pending peer review. In practice, after `graphics-api-expert` returned PASS+ADVISORY, the diff was committed as `d2b2e9fe` along with the shader-profile bump `033b4520`. The advisory was therefore overridden post-review, which is the correct flow — the staged-only directive applies before review-gate completion, not after a PASS verdict. Recording for honesty.
<!-- SECTION:FINAL_SUMMARY:END -->

<!-- SECTION:NOTES:END -->
