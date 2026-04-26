# TASK-138 — RT shadow rays for sun direct lighting (design alignment, pre-implementation)

- User direction (2026-04-26): "try hardware-RT shadow rays for the sun, cost-budgeted against the existing CSM+PCSS path. Current CSM with PCSS measures ~3ms in PIX per user; RT shadows are acceptable if cost ≤ that."
- Reference impl (in-house): `Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl::SampleSkyNEE` (TASK-6.10, commit `e06235ee`) — same shadow-ray flag set + payload-sentinel pattern this task should reuse.
- Reference impl (in-house, gold standard): `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` lines 250-266 — the exact `RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` + `ShadowPayload` + `ShadowMissShader` pattern, with sun-disc cone jitter via `SampleSunDirection`.
- Sibling closure: TASK-6.10 alignment artifact `.alignments/TASK-6.10-sky-nee-secondary-vertex.md` (DXR PSO depth bump 1→2, payload-sentinel discussion, cross-team flagging of `DX12RenderPassResourceService.cpp:498`).
- Author: rendering-researcher; scope owner of `Source/Shaders/HLSL/**` and `Source/ExampleProject/RenderingClient/*Pass.{cpp,h}`. Cross-team edits called out below.

## Status

**Design-ready, NOT IMPLEMENTED.** Stopping pre-code because the deliverable explicitly requires a cost decision that requires PIX or RenderDoc measurement. The dispatch shell available to this agent has neither PIX integration nor any DX12 GPU-timestamp infrastructure inside the engine (no `D3D12_QUERY_HEAP_TYPE_TIMESTAMP` calls anywhere under `Source/Engine`). A CL that swaps CSM for RT without measuring cost violates the task's explicit "cost decision is the most important deliverable" requirement; a CL that ships both paths gated by a `#define` is achievable in a follow-up but expands the surface to a multi-file change touching graphics-api territory.

Recommended next step: user dispatches a fresh rendering-researcher session with PIX-measurement available (or adds a minimal GPU-timestamp helper to `GraphicsHardwareService` first, dispatched to graphics-api-expert). This artifact gives the implementer a complete, paper-faithful design so the implementation CL is a transcription, not a re-derivation.

## Approach: A (standalone DXR pass that produces a per-pixel R8 visibility texture)

Approach B (inline `TraceRay` in `lightPass.comp` directly) requires promoting the LightPass kernel to a DXR-enabled pipeline — substantially larger architectural change. Approach A keeps LightPass as a regular compute pass and bolts a thin RT pass in front of it that consumes only TLAS + GBuffer and writes one UAV. Approach A wins on isolation (the new pass can be enabled/disabled without touching LightPass binding tables or the rest of the rasterizer pipeline).

## Convention cross-check vs `GPUPathTracerRayGen.hlsl` (the gold standard already in the repo)

PT's per-bounce sun NEE block (`GPUPathTracerRayGen.hlsl:250-266`):
```
float3 lightDir = SampleSunDirection(normalize(g_Frame.sun_direction.xyz), Rand2(rng));
ShadowPayload shadow;
shadow.isShadowed = true;
RayDesc shadowRay;
shadowRay.Origin    = payload.hitPos + N * RAY_EPSILON;
shadowRay.Direction = lightDir;
shadowRay.TMin      = RAY_EPSILON;
shadowRay.TMax      = RAY_MAX_DISTANCE;
TraceRay(SceneAS,
         RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
         0xFF, 0, 0, 1, shadowRay, shadow);
if (!shadow.isShadowed) { /* lit */ }
```

Convention details to preserve in TASK-138:
- **`RAY_FLAG_*` triplet**: identical. Cheapest opaque-only visibility test.
- **Payload struct**: `ShadowPayload { bool isShadowed; }` already declared in `common/pathTracerPayload.hlsli`. Reuse via `#include`. Size = 4B; well under the 64B `MaxPayloadSizeInBytes` cap in `DX12RenderPassResourceService.cpp:492`.
- **Miss shader index 1**: `TraceRay(..., 0, 0, 1, ...)` — argument 6 is `MissShaderIndex`. The shadow miss is registered as the second miss (index 1) in the PSO; PT's `m_ShaderFilePaths.m_ShadowMissPath = "GPUPathTracerShadowMiss.hlsl"` plumbing already exists in the engine and is conditionally compiled into the DXR PSO at `DX12RenderPassResourceService.cpp:474-479` and `:521-525` when `m_ShadowMissBuffer` is non-empty. New pass plugs into the same flow.
- **Origin offset**: `+ N * RAY_EPSILON` to escape self-intersection. `RAY_EPSILON = 0.001` in `common/common.hlsl:17`. Use the GBuffer-read normal here, not world-up — sun-shadow rays from grazing-angle pixels will self-intersect without the proper-normal offset.
- **Sun-direction sampling with cone jitter**: `SampleSunDirection(sunDir, xi)` already exists at `GPUPathTracerRayGen.hlsl:159-175`. **Lift it into a shared header** (per "no copy-paste" discipline). New header: `Source/Shaders/HLSL/common/sunSampling.hlsl`. Both PT and the new RT-shadow pass include it. PT-side change is mechanical (delete the local definition, add the include).
- **`SUN_ANGULAR_RADIUS`**: existing `0.000071` rad in `common/common.hlsl:34`. **NOTE:** the task brief says ~0.5° = 0.00872665 rad, which is the real sun's angular *diameter* divided by 2. The engine value (0.000071) is ~125× smaller (~14.7 arcsec) — physically unrealistic, this corresponds to a near-point sun. Both PT (`GPUPathTracerRayGen.hlsl:161`) and the rasterizer's BSDF clamp (`lightPassDirectLighting.hlsl:39`) use it. **Do not change it in this CL** — that's a content-tuning decision out of scope (would shift PT vs rast comparisons and change perceived shadow softness everywhere). The new RT-shadow pass uses the same constant, producing shadows whose softness matches PT's existing tunings. If the user wants larger soft shadows they can bump the constant in a follow-up; that change is then symmetric across PT and rast.
- **Naming the constant**: the task brief asked for `SUN_ANGULAR_HALFANGLE_RAD`. The engine already has `SUN_ANGULAR_RADIUS` with the same role. Keep the existing name to avoid a rename cascade across PT and the BSDF clamp. The brief's preferred name is just terminology; the constant's *meaning* (half-angle of the sun cone) is identical.

Net: the RT-shadow shader for TASK-138 is a near-copy of PT's lines 250-266, but driven from GBuffer reads instead of payload reads.

## File-by-file delta plan

### NEW: `Source/Shaders/HLSL/common/sunSampling.hlsl`

```hlsl
// shadertype=hlsl
#ifndef SUN_SAMPLING_HLSL
#define SUN_SAMPLING_HLSL
#include "common.hlsl"

// Cone-jitter a direction inside the sun's angular cone. SUN_ANGULAR_RADIUS
// (common.hlsl) is the half-angle in radians. xi is a Halton/blue-noise pair
// in [0,1]^2. Returns a unit-length perturbed direction biased on `sunDir`.
//
// Reference: GPUPathTracerRayGen.hlsl SampleSunDirection (the original copy
// of this function — moved here so the RT-shadow pass and PT both consume
// one definition).
float3 SampleSunDirection(float3 sunDir, float2 xi)
{
    float r = sin(SUN_ANGULAR_RADIUS);
    float d = cos(SUN_ANGULAR_RADIUS);

    float phi = TWO_PI * xi.x;
    float cosTheta = 1.0f - xi.y * (1.0f - d);
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up        = abs(sunDir.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent   = normalize(cross(up, sunDir));
    float3 bitangent = cross(sunDir, tangent);

    return normalize(tangent * H.x + bitangent * H.y + sunDir * H.z);
}

#endif // SUN_SAMPLING_HLSL
```

### EDIT: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` (mechanical de-duplication)

Replace lines 159-175 with `#include "common/sunSampling.hlsl"` near the top of the file (after `RayTracingTypes.hlsl` include). No behavioural change.

### NEW: `Source/Shaders/HLSL/SunShadowRTRayGen.hlsl`

Per-pixel sun-visibility raygen. One thread per screen pixel.

```hlsl
// shadertype=hlsl
#include "common/common.hlsl"
#include "common/pathTracerPayload.hlsli"
#include "common/sunSampling.hlsl"

[[vk::binding(0, 0)]] cbuffer PerFrameConstantBuffer : register(b0) { PerFrame_CB g_Frame; }
[[vk::binding(0, 1)]] RaytracingAccelerationStructure SceneAS : register(t0);
[[vk::binding(1, 1)]] Texture2D in_opaquePassRT0 : register(t1); // World position (RGB), validity (A)
[[vk::binding(2, 1)]] Texture2D in_opaquePassRT1 : register(t2); // World normal (RGB), metallic (A)
[[vk::binding(0, 2)]] RWTexture2D<float> out_SunVisibility : register(u0);

// Hash-based per-pixel jitter seed. Mixes pixel coords with frameIndex so
// TAA accumulation across frames produces soft shadows over time.
float2 PixelJitter2D(uint2 pixel, uint frameIndex)
{
    uint3 q = uint3(pixel.x, pixel.y, frameIndex) ^ uint3(0x68E31DA4u, 0xB5297A4Du, 0x1B56C4E9u);
    q = q * 1664525u + 1013904223u;
    q.x ^= q.y * q.z;
    q.y ^= q.z * q.x;
    q.z ^= q.x * q.y;
    q ^= q >> 16u;
    return float2((q.x & 0x00FFFFFFu) / float(0x01000000),
                  (q.y & 0x00FFFFFFu) / float(0x01000000));
}

[shader("raygeneration")]
void RayGenShader()
{
    uint2 pixel = DispatchRaysIndex().xy;

    float4 rt0 = in_opaquePassRT0.Load(int3(pixel, 0));
    if (rt0.w == 0.0)
    {
        // Sky pixel — no surface to shadow. Write 1.0 so any future consumer
        // that reads outside the rendered geometry sees "fully visible" rather
        // than uninitialised memory. LightPass already early-outs on the same
        // sky-test in DecodeGBuffer, so this value is never consumed in practice.
        out_SunVisibility[pixel] = 1.0;
        return;
    }

    float3 positionWS = rt0.xyz;
    float3 normalWS   = normalize(in_opaquePassRT1.Load(int3(pixel, 0)).xyz);

    float3 sunDir = g_Frame.sun_direction.xyz;
    // Loud on data violations (per feedback_no_data_integrity_assumptions.md):
    // a NaN or zero sun direction would produce undefined ray traces. Write
    // 0 (fully shadowed) so the consumer renders a black image — visually
    // distinct from a buggy "fully lit despite shadows" failure mode.
    float sunDirLenSq = dot(sunDir, sunDir);
    if (sunDirLenSq < 1e-8 || isnan(sunDirLenSq))
    {
        out_SunVisibility[pixel] = 0.0;
        return;
    }
    sunDir = sunDir * rsqrt(sunDirLenSq);

    // Per-pixel cone-jittered sun direction. Single sample per pixel; TAA
    // accumulates across frames to soft-shadow the penumbra.
    float2 xi = PixelJitter2D(pixel, g_Frame.frameIndex);
    float3 lightDir = SampleSunDirection(sunDir, xi);

    ShadowPayload shadow;
    shadow.isShadowed = true;

    RayDesc shadowRay;
    shadowRay.Origin    = positionWS + normalWS * RAY_EPSILON;
    shadowRay.Direction = lightDir;
    shadowRay.TMin      = RAY_EPSILON;
    shadowRay.TMax      = RAY_MAX_DISTANCE;

    TraceRay(SceneAS,
             RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
             0xFF, 0, 0, 1, shadowRay, shadow);

    out_SunVisibility[pixel] = shadow.isShadowed ? 0.0 : 1.0;
}
```

### NEW: `Source/Shaders/HLSL/SunShadowRTAnyHit.hlsl`, `SunShadowRTMiss.hlsl`, `SunShadowRTShadowMiss.hlsl`

Three trivial files matching PT's pattern. RT pipelines need a closest-hit, any-hit, and miss registered even when the shadow path uses none of them — `RAY_FLAG_SKIP_CLOSEST_HIT_SHADER` skips the closest-hit shader bytes but the PSO still needs the symbol. Reuse PT's:

- `SunShadowRTAnyHit.hlsl`: identical to `RadianceCacheAnyHit.hlsl` (empty body).
- `SunShadowRTMiss.hlsl`: stub that does nothing. The primary miss (index 0) is unused — `RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH` makes any opaque hit terminate immediately, and the miss goes to the shadow-miss (index 1) per the `1` in `TraceRay(...,0,0,1,...)`.
- `SunShadowRTShadowMiss.hlsl`: identical to `GPUPathTracerShadowMiss.hlsl` (`payload.isShadowed = false;`).
- A trivial closest-hit `SunShadowRTClosestHit.hlsl` — empty `[shader("closesthit")]` body — is also required by the PSO subobject set but never executes due to `SKIP_CLOSEST_HIT_SHADER`.

Five new shader files total. All trivial except `SunShadowRTRayGen.hlsl`.

### NEW: `Source/ExampleProject/RenderingClient/SunShadowRTPass.{h,cpp}`

Pattern-copies `RadianceCacheRaytracingPass.cpp` with binding layout slimmed down (TLAS + GBuffer position + GBuffer normal + per-frame CB → 4 SRV/CBV bindings; one R8 UAV → 1 binding). `m_ShaderProgramComp->m_ShaderFilePaths`:

```cpp
m_ShaderProgramComp->m_ShaderFilePaths.m_RayGenPath     = "SunShadowRTRayGen.hlsl";
m_ShaderProgramComp->m_ShaderFilePaths.m_ClosestHitPath = "SunShadowRTClosestHit.hlsl";
m_ShaderProgramComp->m_ShaderFilePaths.m_AnyHitPath     = "SunShadowRTAnyHit.hlsl";
m_ShaderProgramComp->m_ShaderFilePaths.m_MissPath       = "SunShadowRTMiss.hlsl";
m_ShaderProgramComp->m_ShaderFilePaths.m_ShadowMissPath = "SunShadowRTShadowMiss.hlsl";
```

Render-pass desc: `m_GPUEngineType = Compute`, `m_UseRaytracing = true`, `m_UseOutputMerger = false`. Owns one R8 UAV (`m_SunVisibility`, `R8_UNORM`, viewport-sized) created in `RenderTargetsCreationFunc`. Public getter `GetResult()` returns it. `PrepareCommandList`:

1. Graphics CL: transition opaque RT0/RT1 to ReadOnly; transition `m_SunVisibility` to WriteOnly.
2. Compute CL: bind PerFrameCB + TLAS + opaque RT0/RT1 + `m_SunVisibility`; `DispatchRays(viewport.x, viewport.y, 1)`.

ExampleRenderingClient hookup: register `Setup`, `Initialize`, `Terminate`. In `PrepareCommands` and `ExecuteCommands`, dispatch after `OpaquePass` (needs GBuffer position/normal) and before `LightPass` (LightPass consumes the visibility). Same waiting pattern as `RadianceCacheRaytracingPass` (graphics CL signal, compute CL wait + execute + signal). Compute-queue dispatch — does not block the graphics queue.

### EDIT: `Source/Shaders/HLSL/lightPass.comp` + `Source/Shaders/HLSL/common/lightPassDirectLighting.hlsl`

Replace the `Texture2DArray in_SunShadow` binding (currently `t7`, set 1, descriptor 7) with `Texture2D<float> in_SunShadowVisibility`. The `EvaluateSunLighting` signature changes:

```hlsl
void EvaluateSunLighting(
    in Texture2D in_BRDFLUT,
    in Texture2D in_BRDFMSLUT,
    in SamplerState in_PointSampler,
    in Texture2D<float> in_SunShadowVisibility,    // CHANGED from Texture2DArray
    in MaterialAttributes in_Material,
    in float3 in_PositionWS,
    in float3 in_NormalWS,
    in float3 in_V,
    in uint2 in_ScreenCoord,
    inout float3 io_DirectLuminance,
    inout float3 io_IndirectSeedLuminance)
{
    /* ... sun-direction + L computation unchanged ... */

    float3 l_SunDirect = float3(0.0, 0.0, 0.0);
    float3 l_SunIndirectSeed = float3(0.0, 0.0, 0.0);
    AccumulateLightContribution(/* ... unchanged ... */);

    // Per-pixel visibility produced by SunShadowRTPass. 0 = shadowed,
    // 1 = lit. TAA accumulates the per-frame cone-jittered samples into
    // a soft penumbra without explicit filtering here.
    float l_Visibility = in_SunShadowVisibility.Load(int3(in_ScreenCoord, 0));
    io_DirectLuminance       += l_SunDirect       * l_Visibility;
    io_IndirectSeedLuminance += l_SunIndirectSeed * l_Visibility;
}
```

The CSM `Texture2DArray in_SunShadow` binding, the `in_LinearSampler` parameter, and the `SunShadowResolver` call site all go away in `EvaluateSunLighting`. The `SunShadowResolver` function in `common/shadowResolver.hlsl` becomes orphaned and is deleted in the same CL (per "no orphaned consumers" gate).

Caller-side update in `lightPass.comp`: drop the `in_SunShadow` and `in_LinearSampler` arguments to `EvaluateSunLighting`. The `EvaluateSunLighting` call still passes `in_PointSampler` (used by the BRDF LUT reads, unrelated to shadows).

### EDIT: `Source/ExampleProject/RenderingClient/LightPass.cpp`

Binding-table change at slot index 13 (the `t7` entry):

- Was: `SunShadowGeometryProcessPass::Get().GetResult()` (cascade depth atlas).
- Becomes: `SunShadowRTPass::Get().GetResult()` (R8 visibility).

The HLSL binding name + register stays at `t7` so the slot index doesn't shift; only the bound resource and HLSL declaration type change. ResourceBindingLayoutDescs[13] descriptor type stays `Image`.

`PrepareCommandList`'s pre-LightPass transition list updates: drop the `SunShadowGeometryProcessPass::Get().GetResult()` `WriteOnly → ReadOnly` transition and add the symmetric one for `SunShadowRTPass::Get().GetResult()`.

ExecuteCommands wait list updates: drop `WaitOnGPU(SunShadowGeometryProcessPass::Get()...)` and add `WaitOnGPU(SunShadowRTPass::Get()...)` before `LightPass` execute.

### REMOVE: `SunShadowGeometryProcessPass`, `SunShadowCullingPass`, `SunShadowBlur*Pass`, `common/shadowResolver.hlsl`

If the cost decision is **swap**: delete the four `SunShadow*Pass.{cpp,h}` (Geometry, Culling, BlurEven, BlurOdd) plus the shader header `common/shadowResolver.hlsl`. Drop their `Setup`/`Initialize`/`Terminate`/`PrepareCommands`/`ExecuteCommands` calls from `ExampleRenderingClient.cpp`. Drop their `#include` from `LightPass.cpp` and any other client.

Backlog implication: TASK-66 (point/sphere shadow maps in rasterized pipeline) becomes "RT shadows for point/sphere lights" follow-up — the same SunShadowRT infra extends to point/sphere with a per-light loop in the raygen. **Note this in the closure as a future-task suggestion** (do not implement in this CL; out of scope).

If the cost decision is **keep both** with a runtime gate: leave the SunShadow* passes in place; add a `bool m_UseRTShadows` toggle in `LightPass.cpp` that picks which texture is bound to slot 13 (CSM atlas or RT visibility) per frame. This is more code surface but ships safer.

### EDIT (cross-team — flag for graphics-api-expert): `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp`

`MaxTraceRecursionDepth = 2` (`:503`) is already adequate for this pass. The new pass introduces no nested `TraceRay`, so depth-1 from the raygen suffices and the existing global cap covers it.

`MaxPayloadSizeInBytes = 64` (`:492`) covers `ShadowPayload { bool isShadowed; }` (4B). No change.

The PSO creation flow at `:444-540` already handles the optional shadow-miss subobject and is general — adding a new pass with its own `m_ShadowMissPath` should require no engine-side edit. However, this assumes the global root signature for the new pass mirrors PT's well enough; `graphics-api-expert` should confirm during review that descriptor-set 2 + binding 0 (the visibility UAV) maps cleanly through the pass's auto-generated root signature without requiring a manual override at `LoadRaytracingShaders` or root-sig synthesis call sites. Same answered-yes-by-construction for the `RadianceCacheRaytracingPass` UAVs, so this should "just work".

## Cost-measurement gap (the headline blocker)

The task's primary deliverable is a swap/both/keep decision based on PIX-measured RT-shadow time vs CSM+PCSS time. Three problems:

1. **No engine GPU-timestamp infra.** A grep over `Source/Engine` for `D3D12_QUERY_HEAP|QueryTimestamp|GpuTimer` returns no hits. The engine prints CPU task durations from `Inno::Thread::ExecuteTask` warning lines, but those are CPU-side queue-submission times, not GPU-side pass durations. Adding GPU timestamps is a `GraphicsHardwareService` change — graphics-api-expert territory, multi-file.
2. **PIX is not invokable from the dispatch shell.** `pix.exe -wgpu_capture` requires interactive setup. RenderDoc CLI has the same limitation in this environment.
3. **The user has PIX and the baseline number.** They explicitly cited "i remember seeing 3ms something like that in PIX" — the swap decision needs *post-CL* PIX measurement of the new pass, which only the user can run in their PIX-equipped session.

The cleanest split-of-responsibility:
- **rendering-researcher (this artifact)**: full design, paper-faithful pattern, file diffs ready to drop in.
- **Implementation CL (next session)**: lands the new pass as an *additive* path (CSM stays, RT pass produces visibility into a side texture, `EvaluateSunLighting` keeps reading CSM). User flips a `#define USE_RT_SHADOWS` in `lightPassDirectLighting.hlsl` to A/B the consumer in PIX. Both paths are simultaneously profileable: the CSM cluster (`SunShadowGeometryProcessPass` + the inline PCSS in `SunShadowResolver`) and the new `SunShadowRTPass` show up as distinct PIX events. User picks; the swap-or-delete CL is then a separate small follow-up.
- **graphics-api-expert (optional, if user wants engine-side timing)**: add a thin `GraphicsHardwareService::BeginGpuTimer(name) / EndGpuTimer(name)` API backed by `D3D12_QUERY_HEAP_TYPE_TIMESTAMP` rings. Out of scope for this task.

This split avoids the failure mode of "researcher swaps CSM for RT, ships the CL, user PIXes and finds RT is 6 ms not 3 ms, work has to be reverted". Additive-first is reversible; swap-first is not.

## Validation plan (for the eventual implementation CL)

- **Build green**: `BuildWin.ps1` exit 0 (engine + shader DXIL deploy + RT PSO link).
- **Smoke**: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` exit 0.
- **GBV**: `-gpu_validation -total_frames 10` exit 0 with no D3D12 ERROR / device removed.
- **PIX cost capture (USER-RUN)**:
  - Capture frame, expand timeline. Sum events under `SunShadowGeometryProcessPass` (cascade rasterize) + the LightPass section consuming `SunShadowResolver` (PCSS PCF). Quote total.
  - Same capture, sum events under `SunShadowRTPass`. Quote total.
  - Apply the brief's decision tree.
- **Visual check, GISponza windowed**: 60-frame orbit at `-camera_orbit 20,8,120 -dump_frames 60-119`, archive under `Build/captures/TASK138_post_sponza/`. Cross-check soft-shadow penumbra against `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/gpu_output_0399.png` (PT 300 spp truth — the same `SUN_ANGULAR_RADIUS` cone, so shadow softness should match modulo TAA convergence).
- **Visual check, GITestBox windowed**: confirm no acne / peter-panning on the angled cube faces. The `+ N * RAY_EPSILON` origin offset should make this clean by construction (no per-cascade bias-tuning hack like `MIN_SHADOW_BIAS`/`MAX_SHADOW_BIAS` in the CSM path).
- **Orphan-consumer check**: if SWAP chosen, `grep -r "SunShadowGeometryProcessPass\|SunShadowResolver" Source/` returns no hits.

## What was NOT verified — to call out in the eventual closure

1. **No engine binary built or run from this artifact**. Design only.
2. **Hardware-tier sensitivity untested**. RT shadow cost varies materially between RDNA2/3, Ada, Turing. The user's PIX baseline is on their hardware.
3. **TAA-off behaviour**. Single-jittered RT sample without TAA accumulation will be visibly noisy. If the user disables TAA for any reason, the RT-shadow path needs a fallback (either bump the per-pixel sample count to 4-8, or run a small spatial blur in `SunShadowRTPass` post-trace). Recommend the implementation CL keep this in mind but not solve it pre-emptively — TAA-on is the default and the only configuration the engine ships in.
4. **No closed-form penumbra match against PCSS**. PCSS's penumbra is parameterised by `LIGHT_SIZE = 2.0` in `shadowResolver.hlsl:26` — a free knob with no physical meaning. RT's penumbra is determined by `SUN_ANGULAR_RADIUS` (physical). The two will not match pixel-for-pixel even with identical sun direction; RT's penumbra will look "more correct" but possibly tighter than what users expect from the PCSS look. The visual check should call this out if it shows up.
5. **`SunShadowCullingPass` removal**. If kept-as-cull-only-for-other-uses, deletion is wrong; if it only feeds `SunShadowGeometryProcessPass`, deletion is correct. Verify by grepping its `GetResult()` consumers in Step 1 of the implementation.

## Files to touch (summary)

- NEW shaders: `common/sunSampling.hlsl`, `SunShadowRTRayGen.hlsl`, `SunShadowRTClosestHit.hlsl`, `SunShadowRTAnyHit.hlsl`, `SunShadowRTMiss.hlsl`, `SunShadowRTShadowMiss.hlsl`.
- NEW C++: `SunShadowRTPass.h`, `SunShadowRTPass.cpp` under `Source/ExampleProject/RenderingClient/`.
- EDIT shaders: `GPUPathTracerRayGen.hlsl` (replace `SampleSunDirection` with include), `lightPass.comp` (binding type), `common/lightPassDirectLighting.hlsl` (signature + body).
- EDIT C++: `LightPass.cpp` (binding source), `ExampleRenderingClient.cpp` (Setup/Initialize/Terminate/PrepareCommands/ExecuteCommands hookup).
- DELETE (if SWAP): `SunShadowGeometryProcessPass.{cpp,h}`, `SunShadowCullingPass.{cpp,h}` (verify first), `SunShadowBlur{Even,Odd}Pass.{cpp,h}`, `common/shadowResolver.hlsl`. Drop their references everywhere.
- NO ENGINE EDIT expected (`MaxTraceRecursionDepth = 2` already adequate). Confirm during implementation; if a root-sig synthesis tweak is needed, that's a graphics-api-expert flag.

## Cross-team flags

- `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp`: no edit *expected*. If implementation discovers a root-sig synthesis snag for the new pass's UAV, `graphics-api-expert` review needed (same precedent as TASK-6.10 PSO bump).
- Engine-side GPU timestamps (out of scope for this task): if user wants engine-instrumented timing instead of PIX, that's a separate `graphics-api-expert` task adding `BeginGpuTimer/EndGpuTimer` to `GraphicsHardwareService`.

## RT-infra reuse for TASK-66 (forward-looking)

The same `SunShadowRTPass` shape generalises to point/sphere shadows trivially:

- Sun: one ray per pixel, `SampleSunDirection`-jittered.
- Point: one ray per `(pixel, light)` pair, ray TMax = light distance, no cone jitter (point lights are point-like). Loop inside the raygen over the same `LightCullingPass` per-tile light list LightPass already consumes.
- Sphere: one ray per `(pixel, light)` pair, sample a point on the sphere surface (PT pattern at `GPUPathTracerRayGen.hlsl:345-380`), TMax = distance to the sampled point.

A single `LightShadowRTPass` could produce a per-pixel `Texture2D<uint>` bitmask of which lights are visible (one bit per culled light). Cost is dominated by ray throughput, not shader divergence; tile-culled point-light counts in our scenes (Sponza ~16 lights / tile peak) keep the loop cheap.

Recommend: implement TASK-138 standalone first; then TASK-66 lands as a generalisation that subsumes `SunShadowRTPass` into a `LightShadowRTPass` superset. TASK-138's pass code is throwaway in that sense, but the design + binding shape is preserved.
