# TASK-6.6 — GISponza dimness vs PBR path-tracer reference

- Paper: Boissé et al., "GI-1.0: A Fast Scalable Two-Level Radiance Caching Scheme for Real-Time Global Illumination", AMD Tech. Report 22-10-9831 (`Build/GI1_0.pdf`).
- Reference impl: AMD Capsaicin v1.3 at commit `914b91596cd119eda85fbc1d3c7ee6ac391b1452`, under `Build/reference/Capsaicin/src/core/src/render_techniques/gi1/`.
- In-house impl audited against: `Source/Shaders/HLSL/{RadianceCacheReprojection,RadianceCacheRayGen,RadianceCacheClosestHit,RadianceCacheFilter*,RadianceCacheIntegration,GIDenoise,GIFilter*,lightPass}.{comp,hlsl}`.
- PT ground truth: `Source/Shaders/HLSL/GPUPathTracerRayGen.hlsl` + `GPUPathTracerClosestHit.hlsl` (4-bounce Disney/GGX, sun-disc NEE, sky NEE, point + sphere NEE, multi-lobe importance sampling, Russian roulette). PBR-correct unbiased reference.
- Audit date: 2026-04-26.
- Engine binary: `Bin/RelWithDebInfo/Main.exe` built from `b829a474` (pre-TASK-6.7).

## Headline finding

**The post-TASK-6.5 GISponza rasterized GI is ~10–20% under-energy vs the GPU PT reference.** At the default Sponza Main Camera pose with both paths sharing the same FinalBlend / auto-exposure / tonemap chain:

| Path | mean_L | mean_RGB | dark<8 | dark<24 |
|------|-------:|----------|-------:|--------:|
| PT 300 spp (truth) | 145.80 | (136, 148, **146**) | 2.4% | 4.9% |
| Rast pre-TASK-6.5 (FLOOR active) f60 | 136.91 | (129, 143, **99**) | 0.5% | 1.6% |
| Rast post-TASK-6.5 f60 | 131.05 | (125, 137, **92**) | 6.8% | 15.1% |
| Rast post-TASK-6.5 f100 | 117.32 | (113, 123, **78**) | 22.2% | 25.6% |
| Rast post-TASK-6.5 f119 | 116.11 | (112, 121, **77**) | 23.2% | 26.2% |
| Rast post-TASK-6.5 default-cam f119 | 123.49 | (120, 128, **83**) | 11.9% | 20.9% |

Captures under `Build/captures/TASK6_6_pt_sponza/` with `compare_pt_vs_rast.py` reproducing the numbers; PT convergence verified to ±0.01 luma over the last 5 frames at 300 spp.

The dimness exposed by the LIGHTPASS_AMBIENT_FLOOR removal is **not paper-faithful exposed reality**. Even the PRE-TASK-6.5 rasterized output (with the constant) sat 6–9 luma units below PT truth on this scene. The constant was masking a real under-energy gap, not an honest single-bounce dimness.

## The blue-channel signature

| | R mean | G mean | B mean |
|---|------:|------:|------:|
| PT 300 spp | 136 | 148 | **146** |
| Rast post f60 | 125 | 137 | **92** |
| Rast post f119 | 112 | 121 | **77** |

PT mean_RGB is near-neutral with a small green bias from the engine's atmosphere-scatter sky color (`SkyColor` → `getSkyColor` in `common/skyResolver.hlsl`). Rasterized output is markedly **yellow-orange biased** — high R+G, deficient B by ~50% vs PT.

This is the smoking-gun signature of **missing sky-irradiance / multi-bounce lift in the cool-blue indirect**:
- PT samples sky as a light at every bounce (`GPUPathTracerRayGen.hlsl:276-301`): every surface samples the upper hemisphere, tests visibility, and if the ray escapes to sky it integrates `SkyColor(skyL) * Cook-Torrance · cos · 2π`. So even an interior ceiling that the camera ray hits gets a cool-sky contribution per-vertex.
- Rasterized GI's screen-probe rays only get sky color on a **direct miss** to sky via `RadianceCacheMiss.hlsl:5-33` (which DOES write `getSkyColor(...)` into the payload — verified, not the bug). For an interior surface where the screen probe ray hits the ceiling/wall, the closest-hit at `RadianceCacheClosestHit.hlsl:48-58` reads `in_LightPassOutgoingLuminance` (previous-frame fully-shaded RT). That RT itself is blue-deficient — the indirect feedback bootstraps from the prior frame's dim GI, never accumulating fresh sky energy at the secondary vertex.
- Net effect: only screen-probe rays that geometrically escape Sponza's atrium roof ever pick up sky color, and there are few of those because Sponza's geometry occludes most ray directions. PT samples sky as a light at every interior surface, regardless of whether a bounce ray would have escaped. Hence the blue deficit and the dimness of interior shadow regions.
- This is the previously-identified D4 in the noise-gap audit (`closest-hit reads previous-frame indirect`), which was framed there as an energy-double-count concern. The actual numerical effect is the OPPOSITE: a feedback loop that converges *from below* because each frame's secondary contribution is the previous frame's already-too-dim output, never lifted by independent direct/sky sampling at the secondary vertex.

## Spatial diff

`Build/captures/TASK6_6_pt_sponza/diff_heatmap_default_cam.png` shows red where PT brighter, blue where Rast brighter:
- **Floor and lower atrium glow red** = PT brighter. Multi-bounce + sky lifts the floor that single-bounce SH projection cannot reach.
- **Ceiling area glows red** = PT brighter. Sky NEE shines through the open atrium roof.
- **Curtain edges and pillar edges slightly blue** = Rast brighter. SH-projected GI overshoots at the high-frequency albedo transitions.

`Build/captures/TASK6_6_pt_sponza/side_by_side_default_cam.png` shows rasterized | PT | abs-diff in 1280×720×3.

## Determination

**Under-energy.** PT is brighter than rasterized at every measured pose, by 1.11x–1.26x in luma and 1.5x–2x in blue channel. The gap is concentrated in:
1. Globally flat lift (sky-irradiance) — affects every surface that can see sky through the atrium.
2. Indirect-on-indirect (multi-bounce) — affects floor / shadow-side surfaces that only get light after 2+ bounces.

These are the exact paper-prescribed paths (§2.2 multi-bounce, §2.5 environment lighting) that the auditor's prior pass flagged as missing. They are NOT addressed by TASK-6.7 (which raises sample count to fix noise floor, not energy gap).

## Sensitivity to TASK-6.7

**Partial.** TASK-6.7 raises NUM_SAMPLES from 1 to 16 in `RadianceCacheRayGen.hlsl`. This:
- Closes the under-sampling **noise** gap (the auditor's headline). Each frame's SH projection input is no longer dominated by 63 stale cells + 1 fresh.
- Does NOT introduce any new path for sky radiance to enter the SH projection. The closest-hit still reads `in_LightPassOutgoingLuminance` (no sky-radiance term), and a probe ray that misses geometry still produces zero contribution.
- May reduce a small portion of the energy gap if the prior 1-spp regime had biased-low estimates from undersampled importance sampling — but the structural blue deficit is independent of sample count.

**Predicted post-TASK-6.7 measurement**: PT mean_L 145.8 stays the same (PT is unbiased). Rast mean_L moves from ~123 toward ~128–132 as the sample count fix tightens the integrator's bias. Blue deficit (rast B=83 vs PT B=146) remains ~50%, because no new sky path is wired in.

This audit's conclusions should be re-validated post-TASK-6.7 by running the comparison script against fresh rasterized captures.

## Ranked follow-up tasks

In order of expected impact on the energy gap, with citations:

### 1. Sky-NEE-as-a-light from interior surfaces (HIGH impact, blue deficit)

- **Paper**: §2.5 environment lighting — environment is sampled both as a sky-miss contribution AND as an explicit light at every NEE step, so interior surfaces lit only via bounce-from-cool-sky receive the sky integral even when the bounce ray itself doesn't escape geometry.
- **Capsaicin**: `gi1.comp:2333-2361` `SampleScreenProbes` includes `LightType_Environment` in its NEE light-list (`light_sampler.hlsl::sample`); `gi1.comp:1962-1975` multibounce closest-hit again samples environment as a light at the secondary vertex.
- **Ours**: `RadianceCacheMiss.hlsl:5-33` does write `getSkyColor(...)` into the payload when a probe ray escapes geometry — so the **direct-miss-to-sky path IS wired**. But there is no NEE-for-environment at the closest-hit / secondary-vertex stage. `RadianceCacheClosestHit.hlsl:48-58` only reads `in_LightPassOutgoingLuminance` (the previous-frame screen-space RT) for secondary contribution. Interior surfaces (ceiling, columns, walls) that receive most of their light from sky-bounce-via-floor-and-walls don't get any sky-NEE contribution; they only get the dim previous-frame indirect.
- **Engine fix surface**: at the closest-hit / secondary vertex, add an NEE step that samples a sky direction over the upper hemisphere, traces a shadow ray, and on visibility accumulates `getSkyColor(skyDir) * BRDF * cosTheta * 2π` into the cache — analogous to PT's sky NEE at every bounce (`GPUPathTracerRayGen.hlsl:276-301`).
- **Predicted impact**: closes most of the blue deficit (~50 luma units in B channel) across the interior of the atrium. ~10-15 luma units of mean_L lift.

### 2. World-cache integration for secondary path vertices (MEDIUM impact, multi-bounce lift)

- **Paper**: §2.2 hash-grid world cache feeds bounce-2+ vertices.
- **Capsaicin**: `gi1.comp:2363-2515` `PopulateMultibounceCells` ray-traces from world-cache cells with their hash-grid-stored direct light + sampled secondary BRDF; `gi1.comp:1962-1975` closest-hit reads the filtered hash-grid cache for any depth-N hit.
- **Ours**: `RadianceCacheClosestHit.hlsl:53` reads `in_LightPassOutgoingLuminance.Load(prevScreenCoord)` — uses **previous-frame fully-shaded screen-space RT** as the secondary contribution. This is the prior audit's D4 (energy double-count risk + frame-coupled banding) and also the underlying reason multi-bounce lift is weak: the previous frame's RT already has the dim GI baked in, so the recursion compounds the dimness rather than lifting it from a higher-quality direct-only cache.
- **Engine fix surface**: split `out_lightPassRT0` into a direct-only output and an indirect-only output; have the closest-hit read the direct-only buffer for bounce contribution. Or, port Capsaicin's hash-grid cache so secondary vertices read filtered direct-radiance from a world-space store instead of the screen-space framebuffer.
- **Predicted impact**: 4-6 luma units of mean_L lift in shadow-side / floor regions; reduces the dark<24 fraction from ~25% toward ~10-15%.

### 3. Wider variable-radius blur on relaxed pixels (LOW impact)

- **Paper**: §2.4.3 variable-radius bilateral when probe-cache fallback fails.
- **Capsaicin**: `gi_denoiser.hlsl:46` `GIDenoiser_GetBlurRadius` uses `kGIDenoiser_MaxBlurMask = 8.0`.
- **Ours**: `GIFilterCommon.hlsl:34` `kGIDenoiser_MaxBlurMask = 16.0` — already 2x larger than reference. The blur-radius cap is not the limiting factor for dimness; widening further would over-blur.
- **Note**: this option in the original task brief was speculative — the actual numerical comparison shows we're already over-budget on blur radius, so this is NOT a productive avenue. Defer.

### 4. Restore LIGHTPASS_AMBIENT_FLOOR temporarily (NOT recommended)

- The pre-TASK-6.5 capture with the constant was still 6-9 luma below PT — the constant was masking, not solving. Reintroducing it would be a paper-non-faithful regression.

## Honest limits

1. **Camera-pose mismatch between reference (default Main Camera, no orbit) and the TASK-6.5_post_sponza capture set (orbit poses)**. PT auto-accumulator resets when camera moves, so I cannot get a converged PT reference at an arbitrary orbit pose without engine changes. The default-camera pose was the only pose where I could compare PT-converged against rasterized at the same camera. Cross-pose triangulation (PT-default-cam vs Rast-orbit-frames) shows luma_ratio 1.11x-1.26x consistently across the orbit set, suggesting the gap is pose-invariant.
2. **Auto-exposure shared but adapts to the input stream**. PT and Rast both feed `LuminanceHistogramPass` → `LuminanceAveragePass` → `FinalBlend`, so auto-exposure is in the loop for both. Means PT and Rast see slightly different exposure compensations; converged PT exposure is more representative of the actual scene radiance because it has a more stable input. This is an acceptable comparison setup since both use the same downstream pipeline.
3. **CPU `RayTracer::Execute()` referenced in the task brief is NOT a PBR path tracer.** It's a Pete-Shirley-style AABB-toy ray tracer (`Source/Engine/RayTracer/RayTracer.cpp`): treats every mesh as a `HitableCube`, uses synthetic Lambertian/Metal materials with scalar albedo (no textures), hardcoded sky gradient, and writes `cpu_reference.png` as a downsampled 8x denominator output. Output is a near-uniform green-noise field for GISponza. **The actual ground truth used here is the GPU PT (`GPUPathTracerRayGen.hlsl`), which is a real PBR path tracer with textures, NEE, and multi-bounce.** Recommend the task brief's reference to `World.inl:354` be updated to reference the GPU PT path instead.
4. **Sun illuminance sourced from the same `g_Frame.sun_illuminance.xyz` for PT and rasterized direct lighting**, so direct-lit regions should match in raw radiance. Any PT-vs-rast difference on direct-lit surfaces is from PBR-correct GGX importance sampling vs. the rasterizer's analytic Cook-Torrance — generally small.
5. **TASK-6.7 is in flight** — current binary is pre-TASK-6.7. Comparison numbers will need re-validation after TASK-6.7 lands; predicted impact called out above.
