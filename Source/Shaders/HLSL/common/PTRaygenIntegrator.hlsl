// shadertype=hlsl
#ifndef PT_RAYGEN_INTEGRATOR_HLSL
#define PT_RAYGEN_INTEGRATOR_HLSL

// Path-tracer integrator body extracted from GPUPathTracerRayGen.hlsl.
// Behaviour-preserving extraction — every statement (including all
// PT_HASH_GRID_CACHE_ENABLED + PT_DENOISE_ENABLED #if blocks) is moved
// verbatim from the pre-split inline form. The toggle=0 + toggle=1 DXIL
// byte-identity gate (see TASK-77.2 CL-1 implementation note) verifies
// no semantic drift.
//
// Snippet-split shape: domain-cohesive blocks (GBuffer write, NEE,
// cache Site-3) live in adjacent .hlsli files and are #include-d at
// the call site to keep this TU under the file-size ratchet without
// inflating the integrator with inout-parameter scaffolding. The
// snippets share scope with the surrounding RunPathIntegrator body —
// they are NOT standalone function-call boundaries.
//
// Prerequisites the consumer must include first (in this order):
//   common/common.hlsl
//   common/skyResolver.hlsl
//   common/pathTracerPayload.hlsli
//   common/sunSampling.hlsl
//   PT_HASH_GRID_CACHE_ENABLED + PT_DENOISE_ENABLED #defines, then
//   common/PTHashGridCache.hlsl + common/PTDenoiseShared.hlsl as gated,
//   common/PTRaygenBindings.hlsl   (declares g_Frame / g_FramePrev /
//                                   SceneAS / AccumBuffer / cache UAVs /
//                                   denoiser UAVs)
//   common/PTRaygenHelpers.hlsl    (PCG, Halton, BSDF math, sampling,
//                                   SkyColor, GenerateCameraRay)
//
// The integrator references all of the above via global symbols rather
// than parameters; passing them through would inflate every call site
// and add no robustness benefit.

void RunPathIntegrator(uint2 pixel, uint2 resolution)
{
    uint rng = InitRNG(pixel, g_FrameCount, resolution.x);

    float2 jitter = float2(Halton(g_FrameCount, 2), Halton(g_FrameCount, 3)) - 0.5f;
    RayDesc ray = GenerateCameraRay(pixel, jitter, resolution);

    float3 throughput = float3(1.0f, 1.0f, 1.0f);
    float3 radiance   = float3(0.0f, 0.0f, 0.0f);

#if PT_DENOISE_ENABLED
    // Per-lobe radiance accumulators. The lobe tag is set at the
    // primary-hit BSDF importance sample (the `lobeSample < pDiffuse`
    // branch below) and is immutable for the remainder of the path. NEE
    // contributions at the primary hit always go to diffuse — the
    // primary-hit specular lobe is dominated by mirror-direction
    // estimators which CL-5 will route via specular-NEE. Lobe assignment
    // for indirect contributions follows the path's tag: a path that
    // sampled the specular lobe at bounce 0 carries every subsequent
    // contribution into radianceSpecular, regardless of which lobe is
    // sampled at bounce >= 1. This matches the SVGF "demodulated diffuse
    // / specular" channel separation that CL-2's temporal accumulator
    // expects.
    float3 radianceDiffuse  = float3(0.0f, 0.0f, 0.0f);
    float3 radianceSpecular = float3(0.0f, 0.0f, 0.0f);
    bool   isSpecularPath   = false;
#endif

#if PT_HASH_GRID_CACHE_ENABLED
    // Carry-state for the secondary-bounce cache write (Capsaicin Site-2 /
    // UpdateMultibounceCells, gi1.comp:1962-1975 — adapted to a loop-per-
    // bounce raygen). At iteration N we write `(brdf/pdf_at_(N-1)) * mean_at_N`
    // into the cell touched at vertex N-1, mirroring how Capsaicin folds the
    // tertiary cell's filtered direct radiance into the secondary cell's
    // indirect slot. The brdf/pdf factor is recovered as `throughput /
    // prev_throughput` because throughput already accumulates the BSDF
    // chain. prev_throughput holds throughput at the start of iteration N-1
    // (before the BSDF importance sample), so the ratio at iteration N
    // equals exactly the multiplier applied at end of N-1.
    uint   prev_cell_index = kPTHashGridCache_InvalidId;
    float3 prev_throughput = float3(0.0f, 0.0f, 0.0f);
#endif

    const uint MAX_BOUNCES = 4;

    for (uint bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        PathTracerPayload payload = (PathTracerPayload)0;
        payload.missed = true;

        TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);

        if (payload.missed)
        {
            // Primary miss (camera ray straight to sky) keeps the full HDR
            // value so looking at the sky is correctly bright. Indirect
            // misses land through the sky NEE path below — never here —
            // so indirect rays that escape geometry simply terminate with
            // no additional contribution, avoiding double-counting.
            if (bounce == 0)
            {
                radiance += throughput * SkyColor(ray.Direction);
#if PT_DENOISE_ENABLED
                // Sky pixels: zero RT0 (DecodeGBuffer's sky test —
                // common/lightPassCommon.hlsl:55). The denoiser passes
                // skip pixels where RT0.a == 0, so sky stays untouched
                // by the screen-space filter. Sky luminance is routed to
                // diffuse so a sky-only camera produces a non-zero
                // diffuse channel (specular has no rationale on a sky
                // pixel and would require the lobe tag to mean
                // something at the miss point).
                radianceDiffuse += throughput * SkyColor(ray.Direction);
                u_PTDenoise_PositionInstanceID[pixel] = float4(0.0f, 0.0f, 0.0f, 0.0f);
                u_PTDenoise_NormalMetalness[pixel]    = float4(0.0f, 0.0f, 0.0f, 0.0f);
                u_PTDenoise_AlbedoRoughness[pixel]    = float4(0.0f, 0.0f, 0.0f, 0.0f);
                u_PTDenoise_MotionHitDist[pixel]      = float4(0.0f, 0.0f, 0.0f, 0.0f);
                // Per-lobe radiance UAVs are written once at the
                // function tail (single write site for both hit and
                // miss paths, kept in lockstep with the AccumBuffer
                // composition that already feeds the same values).
#endif
            }
            break;
        }

        float3 N = normalize(payload.normal);
        float3 V = -ray.Direction;

#if PT_HASH_GRID_CACHE_ENABLED
        // Per-vertex direct-lighting accumulator. Mirrors each NEE lobe's
        // contribution before the throughput multiplication so the value
        // matches what Capsaicin's PopulateCells writes (gi1.comp:2087-2095).
        // Touched only at bounce >= 1 (secondary+ vertices); the primary hit
        // is always re-traced fresh — never cached — per the D1 audit note.
        float3 vertexDirectLighting = float3(0.0f, 0.0f, 0.0f);
#endif

        float3 albedo    = payload.albedo;
        float  metalness = payload.metalness;
        // Floor roughness at F0_DIELECTRIC to avoid near-zero roughness numerical instability
        // in GGX microfacet distribution (NDF blows up as alpha → 0).
        float  roughness = max(payload.roughness, F0_DIELECTRIC);

#include "PTRaygenIntegrator_GBufferWrite.hlsli"
#include "PTRaygenIntegrator_NEE.hlsli"
#include "PTRaygenIntegrator_Cache.hlsli"

        // Multi-lobe importance sampling: choose diffuse or specular path
        float3 F0 = lerp(float3(F0_DIELECTRIC, F0_DIELECTRIC, F0_DIELECTRIC), albedo, metalness);
        float NdotV = max(dot(N, V), 0.0f);
        float3 F_approx = FresnelSchlick(NdotV, F0, 1.0f);
        float specWeight = saturate(max(F_approx.x, max(F_approx.y, F_approx.z)) + metalness);
        float diffWeight = (1.0f - specWeight) * (1.0f - metalness);
        float totalWeight = diffWeight + specWeight;
        float pDiffuse = diffWeight / max(totalWeight, 0.0001f);

        float lobeSample = Rand(rng);
        float2 xi = Rand2(rng);
        float3 L;
        float pdf;

        if (lobeSample < pDiffuse)
        {
#if PT_DENOISE_ENABLED
            // Lobe tag is set ONCE at the primary-hit BSDF importance
            // sample and locked for the rest of the path. Subsequent
            // bounces may sample either lobe locally; the path's
            // diffuse/specular bucket is fixed by what the primary hit
            // chose. SVGF demodulated channel separation expects this:
            // a path that started in the specular lobe carries every
            // contribution it brings back into the specular history,
            // regardless of intermediate vertex BSDF importance picks.
            if (bounce == 0u)
                isSpecularPath = false;
#endif
            // Diffuse path: cosine-weighted hemisphere sampling
            L = CosineSampleHemisphere(xi, N);
            float NdotL = max(dot(N, L), 0.0f);
            float cosinePdf = NdotL / PI;

            // Evaluate full BRDF for this direction
            float3 H = normalize(V + L);
            float LdotH = max(dot(L, H), 0.0f);
            float3 F = FresnelSchlick(LdotH, F0, 1.0f);
            float3 kD = (1.0f - F) * (1.0f - metalness);
            float3 brdfDiffuse = kD * albedo / PI;

            pdf = pDiffuse * cosinePdf;
            throughput *= brdfDiffuse * NdotL / max(pdf, 0.0001f);
        }
        else
        {
#if PT_DENOISE_ENABLED
            if (bounce == 0u)
                isSpecularPath = true;
#endif
            // Specular path: GGX importance sampling
            float3 H = ImportanceSampleGGX(xi, N, roughness);
            L = reflect(-V, H);
            float NdotL = dot(N, L);
            if (NdotL <= 0.0f) break;

            float  D     = DistributionGGX(N, H, roughness);
            float  alpha = roughness * roughness;
            float  G     = GeometrySmithGGXCorrelated(NdotL, NdotV, alpha);
            float  NdotH = max(dot(N, H), 0.0f);
            float  VdotH = max(dot(V, H), 0.0f);
            float3 F     = FresnelSchlick(VdotH, F0, 1.0f);

            float ggxPdf = (D * NdotH) / (4.0f * VdotH + 0.0001f);
            pdf = (1.0f - pDiffuse) * ggxPdf;

            float3 specular = D * G * F;
            throughput *= specular * NdotL / max(pdf, 0.0001f);
        }

        // Russian roulette
        float maxComp = max(throughput.x, max(throughput.y, throughput.z));
        if (maxComp < RR_THROUGHPUT_THRESHOLD)
        {
            if (bounce >= 2)
            {
                float survivalProb = max(maxComp, 0.05f);
                if (Rand(rng) > survivalProb) break;
                throughput /= survivalProb;
            }
            else
            {
                break;
            }
        }

        ray.Origin    = payload.hitPos + N * RAY_EPSILON;
        ray.Direction = L;
    }

    float4 prev = AccumBuffer[pixel];
    float  t    = 1.0f / float(g_FrameCount);
#if PT_DENOISE_ENABLED
    // Toggle-on: AccumBuffer still receives the same total radiance.
    // Algebraically `radianceDiffuse + radianceSpecular == radiance` —
    // every NEE / BSDF / cache contribution that lands in `radiance`
    // also lands in exactly one lobe bucket. Summing here keeps the
    // fallback display valid in incremental-landing mode (CL-2 still
    // ships AccumBuffer-driven tonemap; CL-4 swaps this sink for the
    // composed denoised diffuse + specular). Floating-point
    // reassociation may differ from the toggle-off `radiance` value
    // at bit level, by intent — that's why the bypass invariant gate
    // is `ENABLED == 0`.
    float3 clampedRadiance = min(radianceDiffuse + radianceSpecular, 100000.0f);

    // Per-lobe radiance for the CL-2 temporal accumulator (SVGF
    // demodulated diffuse / specular channels). Single-buffered: the
    // temporal pass reads these once, blends into ping-pong history
    // textures, and the next frame's raygen overwrites them. Same
    // `min(., 100000)` clamp the AccumBuffer composition uses so a
    // fireflied lobe value cannot silently destabilise the temporal
    // moment estimator. Alpha=0 reserved (CL-3 ReBLUR-shape may store
    // per-lobe hit distance there).
    u_PTDenoise_RadianceDiffuse[pixel]  = float4(min(radianceDiffuse,  100000.0f), 0.0f);
    u_PTDenoise_RadianceSpecular[pixel] = float4(min(radianceSpecular, 100000.0f), 0.0f);
#else
    float3 clampedRadiance = min(radiance, 100000.0f);
#endif
    AccumBuffer[pixel] = lerp(prev, float4(clampedRadiance, 1.0f), t);
}

#endif // PT_RAYGEN_INTEGRATOR_HLSL
