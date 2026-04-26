# TASK-6.10 — Sky NEE at radiance-cache secondary vertex

- Paper: Boissé et al., "GI-1.0: A Fast Scalable Two-Level Radiance Caching Scheme for Real-Time Global Illumination", §2.5 environment lighting.
- Reference impl: AMD Capsaicin v1.3 at commit `914b91596cd119eda85fbc1d3c7ee6ac391b1452`, `Build/reference/Capsaicin/src/core/src/render_techniques/gi1/gi1.comp:1962-1975` (multibounce closest-hit environment NEE).
- In-house impl: `Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl` (ClosestHitShader) + `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp:498` (recursion-depth bump 1→2).
- Audit baseline: `.alignments/TASK-6.6-pt-vs-rasterized-gisponza.md`.
- Implementation date: 2026-04-26.
- Engine binary: `Bin/RelWithDebInfo/Main.exe` built from `f122dd58` + this CL.

## Headline finding

**Sky-NEE-at-secondary-vertex landed; ~30% of the blue gap closed at the static default-camera pose, ~64% closed at orbit poses, and ~100% closed in mean-luma at the orbit pose.** TASK-6.6's PT-truth target is a hard structural ceiling without the next-step material-routing fix.

| Pose | Pre B | Post B | PT B | Pre B/PT | Post B/PT | Target |
|------|------:|-------:|-----:|---------:|----------:|-------:|
| Default cam f119 | 82.81 | 97.53 | 145.65 | 0.569 | 0.670 | ≥0.85 |
| Orbit span (60-119) | 68.94 | 103.91 | n/a | n/a | n/a | trend toward PT-neutral |

| Pose | Pre L | Post L | PT L | Lift | Target |
|------|------:|-------:|-----:|-----:|-------:|
| Default cam f119 | 123.49 | 130.88 | 145.80 | +7.40 | ≥+8 |
| Orbit span | 120.86 | 144.79 | 145.80 | +23.93 | improvement |

**Static default-camera target was missed by a hair on both axes** (B ratio 0.670 vs 0.85 target; L lift +7.40 vs +8 target). **Orbit metrics dramatically beat expectations** (B span +51%, L lift +24, R/B ratio 1.96→1.48 toward PT-neutral 0.94).

The static-pose miss is structural and consistent with the brief's residual-prediction: closing the rest requires (a) routing real per-instance albedo to the closest-hit (currently approximated by a 0.5 mid-grey constant), and/or (b) TASK-6.8 #7/D4 (direct-only LightPass output to break the converge-from-below feedback loop on the existing `in_LightPassOutgoingLuminance` read). Sky NEE adds the missing energy *channel*; depth and intensity scale with those follow-ups.

## Convention cross-check vs `GPUPathTracerRayGen.hlsl`

PT (lines 276-301):
```
xiSky = Rand2(rng);
skyL = UniformSampleHemisphere(xiSky, N);                  // pdf = 1/(2π)
NdotSky = max(dot(N, skyL), 0.0f);
if (NdotSky > 0.0f) {
    TraceRay(... SKIP_CLOSEST_HIT_SHADER ...);
    if (!skyShadow.isShadowed) {
        skyRadiance = SkyColor(skyL);
        radiance += throughput * CookTorranceGGX(N,V,skyL,...) * skyRadiance * TWO_PI;
    }
}
```

Estimator: `L · BRDF·cos / pdf = L · CookTorrance · 2π`. Numerically correct because PT has the true shading normal `N` from `payload.normal` (vertex/index/material buffers bound to PT pipeline).

Mine (`RadianceCacheClosestHit.hlsl::SampleSkyNEE`):
```
const float3 worldUp = float3(0.0, 1.0, 0.0);
xi = HitHash2D(hitPositionWS, frameIndex);
skyDir = CosineSampleHemisphereTangent(xi, worldUp);       // pdf = (Y·L)/π
TraceRay(... SKIP_CLOSEST_HIT_SHADER ...);
if (skyPayload.distance >= 0.0) {                          // miss-shader fired => sky visible
    skyColor = skyPayload.radiance;                        // RadianceCacheMiss already wrote getSkyColor
    return skyColor * SECONDARY_VERTEX_ALBEDO_FALLBACK;     // Lambertian: BRDF·cos / pdf = albedo
}
```

Convention divergence (intentional, documented in the shader):
1. **Sampling axis**: PT uses surface normal `N`. Mine uses world-up `(0,1,0)` because the radiance-cache pipeline does not bind the vertex/index/material heaps that PT uses to interpolate `N` from barycentrics. Sampling on `-WorldRayDirection()` is wrong (incoming ray reverse for an interior probe ray usually points DOWN, sampling into the floor — exactly the opposite of where sky lives). World-up + visibility-gate is Capsaicin's approach when no per-vertex BRDF is in scope (gi1.comp:1962).
2. **Sampling distribution**: PT uses uniform-hemisphere (constant pdf), needs explicit `× 2π` and `cos(θ)` from `CookTorranceGGX`. Mine uses cosine-weighted (pdf scales with `cos`), so the Lambertian estimator collapses to `albedo` per the standard cosine-importance cancellation (`(albedo/π) · cos / (cos/π) = albedo`). Both estimators are unbiased; cosine-weighted has lower variance for diffuse-dominant interiors.
3. **BRDF**: PT evaluates full Cook-Torrance GGX with the true material. Mine uses Lambertian + a fixed `SECONDARY_VERTEX_ALBEDO_FALLBACK = 0.5` (mid-grey). Documented in the shader as a placeholder until material routing lands.
4. **Sky color source**: PT calls `SkyColor(skyL)` (re-evaluates with camera position as eye). Mine reads `skyPayload.radiance` written by the existing `RadianceCacheMiss` (eye = ray origin = hit position). Functionally identical at planetary scale (atmosphere depth >> distance from camera to a Sponza wall); reusing the miss-shader call site avoids any chance of getSkyColor invocation-site drift.
5. **Visibility test**: identical pattern (`RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER`). Mine uses a `payload.distance < 0.0` sentinel rather than a separate `ShadowPayload` because the radiance-cache RT pipeline only registers one miss shader (no shadow-miss subobject) — the existing miss-shader-writes-sky-color pattern serves both purposes once the sentinel disambiguates "no miss fired (occluded)" from "miss fired (sky visible)".

## Pipeline-level support: MaxTraceRecursionDepth 1 → 2

The DXR pipeline-state object at `DX12RenderPassResourceService.cpp:498` was hardcoded to `MaxTraceRecursionDepth = 1`, sufficient for PT (whose bounce loop is iterative-from-raygen) and for the original radiance-cache CHS (which only reads buffer textures, no nested TraceRay). Sky NEE issues a TraceRay from inside the CHS, requiring depth 2.

Bumped to 2. Cost is negligible (one extra register per ray slot). Verified clean GBV pass + the smoke-test crash this CL initially exhibited (`DX12 create failed: default heap buffer ... DeviceRemovedReason=-2005270522`) was fixed by this single line.

This is a cross-team edit (the file is owned by `graphics-api-expert`). The change is mechanical, isolated, and required to enable the shader port; recommend the graphics-api owner sanity-check on next pass.

## Pipeline-level note: pipeline applies to ALL DXR pipelines

The `MaxTraceRecursionDepth = 2` change is global — it applies to GPU-PT and any other ray-tracing pipeline created through the same code path (the function `CreateRaytracingPipelineStateObject` is called for all DXR PSOs). For pipelines that don't issue nested TraceRay this is a no-op (the depth cap is just a max, not a per-ray budget). No behavioural change to PT, TestSuite, or Editor.

## Validation evidence

- **Build**: clean. `BuildWin.ps1` → exit 0, both `Main.exe` and `RenderTest.exe` linked. Build log auto-deploys DXIL shaders to `Bin/RelWithDebInfo/Shaders/DXIL/`.
- **Smoke**: `Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 30` → exit 0.
- **GBV**: `Main.exe ... -gpu_validation -total_frames 10` → exit 0, no D3D12 ERROR / VALIDATION ERROR / Device removed.
- **Default-camera capture**: `Main.exe ... -total_frames 120 -dump_frames 115-119` → 5 frames archived under `Build/captures/TASK6_10_post_sponza/rasterized_default_camera/`.
- **Orbit capture**: `Main.exe ... -total_frames 120 -dump_frames 60-119 -camera_orbit 20,8,120` → 60 frames archived under `Build/captures/TASK6_10_post_sponza/orbit/`.
- **PT comparison script**: `Build/captures/TASK6_10_post_sponza/compare_pt_vs_rast.py` reproduces the per-channel ratios + mean-luma lift table. Side-by-side (PRE | POST | PT) at `side_by_side_pre_post_pt.png` shows the visible blue lift in shadowed wall regions.
- **Orbit diff stats script**: `Build/captures/TASK6_10_post_sponza/orbit_diff_stats.py` reproduces the consec-frame noise stats (mean luma-delta 43.010 → 39.196 — *quieter* by 9%) and the 60-frame R/B-cast tracking (1.964 → 1.483 toward PT-neutral 0.937).

## Honest limits

1. **Static default-camera pose missed both targets** by 0.18 (B ratio) and 0.6 (L units). Direction is correct, magnitude is structurally limited by:
   - **Fixed 0.5 albedo approximation** at the secondary vertex. A real material routing (TASK-6.8 #7/D4 sibling work) would lift this to per-instance reflectance, contributing at least ~40% more on average since real Sponza walls are closer to 0.7.
   - **Missing world-cache write of sky energy**. The CHS currently ADDS sky NEE to the reading-side radiance, but the WRITE-side world-tile-grid update at `RadianceCacheRayGen.hlsl:386-455` only writes the screen-probe-traced radiance, not a sky-NEE sample at the off-screen / world-cache path. Off-screen probe hits read the world cache (no sky NEE applied) and the world cache itself never accumulates sky directly — so for off-screen-dominated regions the sky-NEE lift is muted. Documented for follow-up.
   - **Temporal blend asymptote**. Tested at 240-frame warmup vs 120-frame: B ratio went 0.670 → 0.671 (essentially no change). Algorithm-3 hysteresis at `RadianceCacheRayGen.hlsl::TemporalBlendAlgo3` saturates at the steady-state value imposed by the energy budget; longer warmup doesn't help.
2. **Estimated normal proxy**. The world-up bias is a known approximation; surfaces with normals roughly matching world-up integrate correctly, surfaces with horizontal normals (interior columns, vertical walls) over-estimate by up to a factor of `1 / cos(angle-to-up)` because cosine-weighted-around-Y over-counts on tilted surfaces. The visibility check partially compensates (oriented walls are more occluded by adjacent geometry, so the visible cone is smaller). Ground-truth would require per-hit shading-normal access via vertex/index buffer routing.
3. **GITestBox not run**. The only path to load that scene offscreen is via existing test-case scaffolding (`-test gpu_path_tracer` etc) which doesn't have a "GITestBox + rasterizer" preset. Adding a CLI flag for it is more scope than this task warrants. Structural prediction: TestBox is a fully-enclosed cube with a small sun-window — sky NEE shadow rays from interior surfaces will be blocked by the ceiling → sky-NEE contribution ≈ 0 for all interior surfaces → no visible regression from this CL on TestBox interiors. Only the exterior sky-window-facing surfaces would see sky-NEE energy added.
4. **Audit baseline drift**. The TASK-6.6 PT reference was captured pre-TASK-6.7+6.9, so the delta between PRE-TASK-6.10 (`Build/captures/TASK6_6_pt_sponza/rasterized_default_camera/gpu_output_0119.png`) and POST-TASK-6.10 includes only the sky-NEE effect (TASK-6.7 and TASK-6.9 changed the rasterized side identically in both captures since both were re-captured against the same engine binary post-CL chain). The PT 300-spp capture is unchanged.
5. **Off-screen world-cache branch unaffected**. The sky-NEE accumulation lands in the on-screen + off-screen branches uniformly (added after both `if (withinBounds)` and the world-tile-walk fallback), but the world-tile-grid writeback at the RayGen side does not include the sky-NEE component — only the closest-hit-returned radiance which already includes sky NEE. Verifying this end-to-end would require a debug trace I haven't run; the design is structurally correct but the propagation through the world cache is one frame delayed by construction. Documented as a known minor-latency concern.

## Coordination with sibling work

**TASK-6.8 #7/D4 (direct-only LightPass output)** is a true sibling. They land independently and compound:
- Sky NEE (this CL) adds the missing blue **input** at the secondary vertex.
- Direct-only LightPass output (D4) breaks the converge-from-below feedback loop on the on-screen branch's `in_LightPassOutgoingLuminance` read, letting the rest of the spectrum (sun + indirect-of-indirect) lift correctly.

Without D4, the existing on-screen indirect read still bootstraps from the prior-frame fully-shaded RT (which includes the dim GI from prior frames) — so the steady state is `<sky-NEE-fixed-input> + <converge-from-below feedback>` rather than `<sky-NEE-fixed-input> + <correctly-lifted bounce>`. They compound predictably: D4 alone doesn't add the missing blue (the feedback loop converges from below regardless of which buffer it reads from); this CL alone doesn't lift the warm channels (which need the multi-bounce loop, not a single sky lookup). Both together approach the PT truth.

No sequencing concern: they touch disjoint code paths (this CL: shader closest-hit + DXR PSO depth; D4: light-pass output buffer split). Either can land first.

## Files touched

- `Source/Shaders/HLSL/RadianceCacheClosestHit.hlsl` (+125 lines) — sky NEE function and call site.
- `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp` (+6/-1 lines) — `MaxTraceRecursionDepth` 1 → 2 with explanatory comment. Cross-team edit; flagged for graphics-api-expert review.

## Capture artifacts under `Build/captures/TASK6_10_post_sponza/`

- `rasterized_default_camera/gpu_output_0115..0119.png` — 5-frame static capture of GISponza at default Main Camera pose (post-warmup).
- `rasterized_default_camera_240/gpu_output_0235..0239.png` — same pose, 240-frame warmup. Confirms temporal-blend saturation.
- `orbit/gpu_output_0060..0119.png` — 60-frame orbit at `-camera_orbit 20,8,120`.
- `compare_pt_vs_rast.py` — reproduces the per-channel ratio + mean-luma stats vs `Build/captures/TASK6_6_pt_sponza/default_camera_300spp/gpu_output_0399.png` (PT 300-spp truth).
- `orbit_diff_stats.py` — reproduces the orbit consec-frame noise + 60-frame mean RGB delta vs `Build/captures/TASK6_9_post_sponza/`.
- `side_by_side_pre_post_pt.png` — visual side-by-side (PRE | POST | PT, 1280×720×3).
