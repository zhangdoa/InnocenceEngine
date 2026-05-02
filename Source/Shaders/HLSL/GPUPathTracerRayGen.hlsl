// shadertype=hlsl
#include "common/common.hlsl"
#include "common/skyResolver.hlsl"
#include "common/pathTracerPayload.hlsli"
#include "common/sunSampling.hlsl"

// Master toggle for the secondary-vertex hash-grid radiance cache. When 0,
// the cache code below strips at compile time and this shader produces the
// same DXIL/SPIR-V as the cache-off baseline at HEAD 10d7b158 (the bypass
// invariant — visual-validation.md §3b). The C++ side mirrors this in
// Source/ExampleProject/RenderingClient/HashGridCacheConstants.h::ENABLED;
// both must agree.
//
// Toggle-on adds the Site-3 read pattern (Capsaicin's glossy-reflections
// shape — gi1.comp:2865-2900) plus a Site-2 / UpdateMultibounceCells-style
// secondary-bounce write (gi1.comp:1962-1975). At every secondary+ vertex
// we (1) atomic-add this-vertex direct light into the cell's scratch slot,
// (2) read the resolved running mean from ValueBuffer, and if it carries
// samples (3) atomic-add the BRDF/pdf-modulated mean back into the previous
// vertex's cell — the indirect-lobe feedback that turns the cache from a
// direct-only sketch into a full outgoing-radiance estimator — before
// terminating the path with throughput * (radianceSum / sampleCount).
//
// The companion PTHashGridCacheUpdateTilesPass runs each frame BEFORE this
// raygen and resolves the previous frame's atomic scratch deltas
// (UpdateCellValueBuffer) into ValueBuffer with a 16-sample-cap running
// mean (Capsaicin gi1.comp:2160-2225 mip-0 block). Reads here therefore
// see a stable, capped estimator — not the unbounded scratch sum the
// previous CL was forced to sample. The mip-cascade (mip 1-3 box filter)
// and PurgeTiles (50-frame decay) are still deferred to follow-up CLs.
//
// Reference: Capsaicin GI-1.0 hash_grid_cache.hlsl + gi1.comp secondary-
// vertex sites. Full audit at .alignments/TASK-77.1-rework-paper-port-audit.md.
#define PT_HASH_GRID_CACHE_ENABLED 0

#if PT_HASH_GRID_CACHE_ENABLED
#include "common/PTHashGridCache.hlsl"
#endif

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0) { PerFrame_CB g_Frame; }

[[vk::binding(1, 0)]]
cbuffer FrameCountCB : register(b1) { uint g_FrameCount; }

[[vk::binding(2, 0)]]
cbuffer LightCountCB : register(b2) { uint g_PointLightCount; uint g_SphereLightCount; uint g_LightCountPad0; uint g_LightCountPad1; }

#if PT_HASH_GRID_CACHE_ENABLED
[[vk::binding(3, 0)]]
cbuffer HashGridCacheCB : register(b3) { PTHashGridCacheCB_t g_HashGridCacheConstants; }
#endif

[[vk::binding(0, 1)]]
RaytracingAccelerationStructure SceneAS : register(t0);

[[vk::binding(5, 1)]]
StructuredBuffer<PointLight_CB> g_PointLights : register(t5);

[[vk::binding(6, 1)]]
StructuredBuffer<SphereLight_CB> g_SphereLights : register(t6);

[[vk::binding(0, 2)]]
RWTexture2D<float4> AccumBuffer : register(u0);

#if PT_HASH_GRID_CACHE_ENABLED
[[vk::binding(1, 2)]]
RWStructuredBuffer<uint>  g_HashGridCache_HashBuffer            : register(u1);

[[vk::binding(2, 2)]]
RWStructuredBuffer<uint>  g_HashGridCache_DecayTileBuffer       : register(u2);

[[vk::binding(3, 2)]]
RWStructuredBuffer<uint>  g_HashGridCache_UpdateCellValueBuffer : register(u3);

[[vk::binding(4, 2)]]
RWStructuredBuffer<uint2> g_HashGridCache_ValueBuffer           : register(u4);
#endif

uint PCG(inout uint state)
{
    uint oldState = state;
    state = oldState * 747796405u + 2891336453u;
    uint word = ((oldState >> ((oldState >> 28u) + 4u)) ^ oldState) * 277803737u;
    return (word >> 22u) ^ word;
}

float Rand(inout uint rng) { return float(PCG(rng)) / 4294967296.0f; }

float2 Rand2(inout uint rng)
{
    return float2(Rand(rng), Rand(rng));
}

uint InitRNG(uint2 pixel, uint frame, uint width)
{
    return pixel.x + pixel.y * width + frame * 1073741827u;
}

float Halton(uint index, uint base)
{
    float result = 0.0f, f = 1.0f;
    while (index > 0) { f /= float(base); result += f * float(index % base); index /= base; }
    return result;
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denom * denom);
}

float GeometrySmithGGXCorrelated(float NdotL, float NdotV, float alpha)
{
    float alpha2 = alpha * alpha;
    float lambdaV = NdotL * sqrt(NdotV * NdotV * (1.0f - alpha2) + alpha2);
    float lambdaL = NdotV * sqrt(NdotL * NdotL * (1.0f - alpha2) + alpha2);
    return 0.5f / max(lambdaV + lambdaL, 0.0001f);
}

float3 FresnelSchlick(float cosTheta, float3 F0, float F90)
{
    return F0 + (F90 - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float DisneyDiffuse2015(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float F_l = pow(1.0f - NdotL, 5.0f);
    float F_v = pow(1.0f - NdotV, 5.0f);
    float retroReflect = 2.0f * LdotH * LdotH * linearRoughness;
    float FLambert = (1.0f - 0.5f * F_l) * (1.0f - 0.5f * F_v);
    float FRetroReflection = retroReflect * (F_l + F_v + F_l * F_v * (retroReflect - 1.0f));
    return FLambert + FRetroReflection;
}

float3 CookTorranceGGX(float3 N, float3 V, float3 L, float3 albedo, float metalness, float roughness)
{
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float LdotH = max(dot(L, H), 0.0f);

    float3 F0 = lerp(float3(F0_DIELECTRIC, F0_DIELECTRIC, F0_DIELECTRIC), albedo, metalness);
    float alpha = roughness * roughness;

    float  D = DistributionGGX(N, H, roughness);
    float  G = GeometrySmithGGXCorrelated(NdotL, NdotV, alpha);
    float3 F = FresnelSchlick(LdotH, F0, 1.0f);

    // Specular: D * G * F (correlated Smith already includes 1/(4*NdotL*NdotV) denominator)
    float3 specular = D * G * F;

    // Diffuse: Disney 2015 Burley with energy conservation
    float3 kD = (1.0f - F) * (1.0f - metalness);
    float  diffuseTerm = DisneyDiffuse2015(NdotV, NdotL, LdotH, roughness);
    float3 diffuse = kD * albedo * diffuseTerm / PI;

    return (diffuse + specular) * NdotL;
}

float3 ImportanceSampleGGX(float2 xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = TWO_PI * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up    = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float3 UniformSampleHemisphere(float2 xi, float3 N)
{
    // Uniform over the hemisphere above N. pdf = 1 / (2π).
    float phi = TWO_PI * xi.x;
    float cosTheta = xi.y;
    float sinTheta = sqrt(max(1.0f - cosTheta * cosTheta, 0.0f));

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up    = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float3 CosineSampleHemisphere(float2 xi, float3 N)
{
    float phi = TWO_PI * xi.x;
    float cosTheta = sqrt(1.0f - xi.y);
    float sinTheta = sqrt(xi.y);

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up    = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

// SampleSunDirection lives in common/sunSampling.hlsl now (TASK-138 — shared
// with SunShadowRTRayGen.hlsl). Behaviour-preserving extraction.

float3 SkyColor(float3 dir)
{
    float3 lightdir = normalize(g_Frame.sun_direction.xyz);
    float planetRadius = 6371e3;
    float atmosphereHeight = 100e3;
    float3 eye_position = g_Frame.camera_posWS.xyz + float3(0.0f, planetRadius, 0.0f);

    return getSkyColor(dir, eye_position, lightdir, g_Frame.sun_illuminance.xyz, planetRadius, atmosphereHeight);
}

RayDesc GenerateCameraRay(uint2 pixel, float2 jitter, uint2 resolution)
{
    float2 uv = (float2(pixel) + 0.5f + jitter) / float2(resolution);
    uv.y = 1.0f - uv.y;
    float2 ndc = uv * 2.0f - 1.0f;

    float4 viewPos = mul(float4(ndc.x, ndc.y, 1.0f, 1.0f), g_Frame.p_inv);
    viewPos /= viewPos.w;
    float3 worldPos = mul(float4(viewPos.xyz, 0.0f), g_Frame.v_inv).xyz;

    RayDesc ray;
    ray.Origin    = g_Frame.camera_posWS.xyz;
    ray.Direction = normalize(worldPos);
    ray.TMin      = RAY_EPSILON;
    ray.TMax      = RAY_MAX_DISTANCE;
    return ray;
}

[shader("raygeneration")]
void RayGenShader()
{
    uint2 pixel     = DispatchRaysIndex().xy;
    uint2 resolution = DispatchRaysDimensions().xy;

    uint rng = InitRNG(pixel, g_FrameCount, resolution.x);

    float2 jitter = float2(Halton(g_FrameCount, 2), Halton(g_FrameCount, 3)) - 0.5f;
    RayDesc ray = GenerateCameraRay(pixel, jitter, resolution);

    float3 throughput = float3(1.0f, 1.0f, 1.0f);
    float3 radiance   = float3(0.0f, 0.0f, 0.0f);

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
                radiance += throughput * SkyColor(ray.Direction);
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

        // Direct sun lighting with soft shadow (jittered sun disk)
        float3 lightDir = SampleSunDirection(normalize(g_Frame.sun_direction.xyz), Rand2(rng));
        float3 lightIlluminance = g_Frame.sun_illuminance.xyz;

        ShadowPayload shadow;
        shadow.isShadowed = true;
        RayDesc shadowRay;
        shadowRay.Origin    = payload.hitPos + N * RAY_EPSILON;
        shadowRay.Direction = lightDir;
        shadowRay.TMin      = RAY_EPSILON;
        shadowRay.TMax      = RAY_MAX_DISTANCE;
        TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                 0xFF, 0, 0, 1, shadowRay, shadow);

        if (!shadow.isShadowed)
        {
            radiance += throughput * CookTorranceGGX(N, V, lightDir, albedo, metalness, roughness) * lightIlluminance;
#if PT_HASH_GRID_CACHE_ENABLED
            vertexDirectLighting += CookTorranceGGX(N, V, lightDir, albedo, metalness, roughness) * lightIlluminance;
#endif
        }

        // Sky NEE. Visibility-gated environment sampling: cosine-weighted
        // hemisphere sample, shadow ray, and if the sample escapes scene
        // geometry the sky's HDR radiance contributes. Replaces the old
        // "sky on any indirect miss" path — occluded rays (Sponza's roof
        // etc.) contribute zero; visible sky (open atrium, skybox scenes)
        // still lights the surface correctly. No MIS with BSDF sampling
        // because indirect miss no longer carries sky radiance (see miss
        // branch above).
        {
            float2 xiSky = Rand2(rng);
            float3 skyL = UniformSampleHemisphere(xiSky, N);
            float  NdotSky = max(dot(N, skyL), 0.0f);
            if (NdotSky > 0.0f)
            {
                ShadowPayload skyShadow;
                skyShadow.isShadowed = true;
                RayDesc skyRay;
                skyRay.Origin    = payload.hitPos + N * RAY_EPSILON;
                skyRay.Direction = skyL;
                skyRay.TMin      = RAY_EPSILON;
                skyRay.TMax      = RAY_MAX_DISTANCE;
                TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                         0xFF, 0, 0, 1, skyRay, skyShadow);
                if (!skyShadow.isShadowed)
                {
                    // Uniform-hemisphere pdf = 1/(2π). CookTorranceGGX
                    // already returns BRDF·cos, so the estimator
                    //   L·(BRDF·cos)/pdf = L·CookTorrance·2π
                    // no cos-division hazard at grazing angles.
                    float3 skyRadiance = SkyColor(skyL);
                    radiance += throughput * CookTorranceGGX(N, V, skyL, albedo, metalness, roughness) * skyRadiance * TWO_PI;
#if PT_HASH_GRID_CACHE_ENABLED
                    vertexDirectLighting += CookTorranceGGX(N, V, skyL, albedo, metalness, roughness) * skyRadiance * TWO_PI;
#endif
                }
            }
        }

        // Point light NEE
        for (uint ptIdx = 0; ptIdx < g_PointLightCount; ptIdx++)
        {
            float3 ptPos     = g_PointLights[ptIdx].position.xyz;
            float  ptRadius  = g_PointLights[ptIdx].luminousFlux.w;
            float3 ptFlux    = g_PointLights[ptIdx].luminousFlux.xyz;

            float3 toLight = ptPos - payload.hitPos;
            float  dist    = length(toLight);
            if (dist > ptRadius)
                continue;

            float3 L    = toLight / dist;
            float NdotL = max(dot(N, L), 0.0f);
            if (NdotL <= 0.0f)
                continue;

            ShadowPayload ptShadow;
            ptShadow.isShadowed = true;
            RayDesc ptShadowRay;
            ptShadowRay.Origin    = payload.hitPos + N * RAY_EPSILON;
            ptShadowRay.Direction = L;
            ptShadowRay.TMin      = RAY_EPSILON;
            ptShadowRay.TMax      = dist - 0.002f;
            TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                     0xFF, 0, 0, 1, ptShadowRay, ptShadow);

            if (!ptShadow.isShadowed)
            {
                float  attenuation = 1.0f / max(dist * dist, 0.0001f);
                float3 irradiance  = ptFlux * attenuation;
                radiance += throughput * CookTorranceGGX(N, V, L, albedo, metalness, roughness) * irradiance;
#if PT_HASH_GRID_CACHE_ENABLED
                vertexDirectLighting += CookTorranceGGX(N, V, L, albedo, metalness, roughness) * irradiance;
#endif
            }
        }

        // Sphere light NEE — sample a random point on the sphere surface so
        // neighbouring pixels / frames land on different directions, giving
        // soft shadows after accumulation. Previous implementation sampled
        // the center (hard shadows — indistinguishable from a point light).
        // Uniform-area sampling over the full sphere; a visible-hemisphere
        // importance sampler would converge faster but the math is larger.
        // TASK-67 AC #2.
        for (uint spIdx = 0; spIdx < g_SphereLightCount; spIdx++)
        {
            float3 spCenter     = g_SphereLights[spIdx].position.xyz;
            float  spSphereRad  = g_SphereLights[spIdx].luminousFlux.w;
            float3 spFlux       = g_SphereLights[spIdx].luminousFlux.xyz;

            // Uniform point on unit sphere (Marsaglia's z/azimuth method).
            float2 xiSp = Rand2(rng);
            float  zSp  = 2.0f * xiSp.x - 1.0f;
            float  phi  = 2.0f * PI * xiSp.y;
            float  rSp  = sqrt(max(1.0f - zSp * zSp, 0.0f));
            float3 nLight  = float3(rSp * cos(phi), rSp * sin(phi), zSp);
            float3 pLight  = spCenter + spSphereRad * nLight;

            float3 toLight = pLight - payload.hitPos;
            float  dist    = length(toLight);
            float3 L       = toLight / dist;
            float NdotL    = max(dot(N, L), 0.0f);
            float cosLight = max(dot(nLight, -L), 0.0f);
            if (NdotL <= 0.0f || cosLight <= 0.0f)
                continue;

            ShadowPayload spShadow;
            spShadow.isShadowed = true;
            RayDesc spShadowRay;
            spShadowRay.Origin    = payload.hitPos + N * RAY_EPSILON;
            spShadowRay.Direction = L;
            spShadowRay.TMin      = RAY_EPSILON;
            // Stop just before the sampled surface point — no self-hit on the sphere.
            spShadowRay.TMax      = max(dist - RAY_EPSILON, RAY_EPSILON);
            TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                     0xFF, 0, 0, 1, spShadowRay, spShadow);

            if (!spShadow.isShadowed)
            {
                // Treat spFlux as total radiant flux Φ of a uniform diffuse
                // emitter. Surface radiance L_e = Φ / (π · 4π r²). Area PDF
                // = 1 / (4π r²). Converting to solid-angle at the shading
                // point: E = L_e · cosLight · (4π r²) / dist² · (1/sample).
                // Simplifies to: incomingRadiance = Φ · cosLight / (π · dist²).
                float  geomTerm = cosLight / max(dist * dist, 0.0001f);
                float3 incoming = spFlux * geomTerm / PI;
                radiance += throughput * CookTorranceGGX(N, V, L, albedo, metalness, roughness) * incoming;
#if PT_HASH_GRID_CACHE_ENABLED
                vertexDirectLighting += CookTorranceGGX(N, V, L, albedo, metalness, roughness) * incoming;
#endif
            }
        }

#if PT_HASH_GRID_CACHE_ENABLED
        // Site-3 read + secondary-vertex write at every secondary+ vertex.
        // Bounce 0 is never cached: primary visibility is always re-traced
        // fresh per the D1 audit note. The write populates UpdateCellValueBuffer
        // (per-frame atomic scratch — UpdateTiles consumes and clears it
        // each frame); the read replaces the remaining integration tail with
        // the cell's mean radiance scaled by the current path throughput —
        // Capsaicin's glossy-reflections pattern adapted to a loop-per-
        // bounce raygen (gi1.comp:2865-2900).
        //
        // Two writes happen at this site:
        //   (a) THIS-vertex direct light (vertexDirectLighting) into the
        //       cell at this vertex — the analogue of Capsaicin PopulateCells
        //       (gi1.comp:2087-2095) writing payload.lighting to the
        //       secondary cell's UpdateCellValueBuffer.
        //   (b) PREVIOUS-vertex secondary-bounce contribution: when the
        //       cell at this vertex carries a usable mean, write
        //       `bsdf_over_pdf_at_(N-1) * mean` into the cell at the
        //       previous vertex. This mirrors Capsaicin's UpdateMultibounceCells
        //       (gi1.comp:1962-1975) which folds the tertiary cell's
        //       filtered direct radiance into the secondary cell's indirect
        //       slot weighted by the bounce-1→2 BRDF/pdf. Capsaicin keeps a
        //       separate UpdateCellValueIndirectBuffer for that contribution;
        //       the D1 collapse to a single buffer accepts the running-mean
        //       blend across direct + indirect lobes.
        //
        // The previous CL wrote (a) only, so cells stored direct-only content
        // and the read replaced the indirect-lobe tail with direct lighting
        // alone — biased high. Adding (b) lets the cache mean converge to a
        // faithful outgoing-radiance estimator: each frame the next bounce's
        // cache lookup feeds back into the previous cell, and the running
        // mean stabilises around the integrated-indirect contribution.
        bool cacheTerminated = false;
        if (bounce >= 1u)
        {
            PTHashGridCache_Data data;
            data.eye_position = g_Frame.camera_posWS.xyz;
            data.hit_position = payload.hitPos;
            data.direction    = ray.Direction;
            data.hit_distance = length(payload.hitPos - ray.Origin);

            uint  tile_index;
            bool  is_new_tile;
            uint  cell_index = PTHashGridCache_InsertCell(g_HashGridCacheConstants, data,
                                                         g_HashGridCache_HashBuffer,
                                                         tile_index, is_new_tile);

            if (cell_index != kPTHashGridCache_InvalidId)
            {
                // Bump tile-decay timestamp so PurgeTiles (deferred to a later
                // CL) keeps the tile alive while it is being touched.
                uint prev_decay;
                InterlockedExchange(g_HashGridCache_DecayTileBuffer[tile_index], g_FrameCount, prev_decay);

                // (a) THIS-vertex direct light (Capsaicin PopulateCells write).
                uint4 quantizedDirect = PTHashGridCache_QuantizeRadiance(vertexDirectLighting);
                uint  prev_atomic;
                InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * cell_index + 0u], quantizedDirect.x, prev_atomic);
                InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * cell_index + 1u], quantizedDirect.y, prev_atomic);
                InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * cell_index + 2u], quantizedDirect.z, prev_atomic);
                InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * cell_index + 3u], quantizedDirect.w, prev_atomic);

                // Read the resolved running mean from ValueBuffer, populated
                // by PTHashGridCacheUpdateTilesPass earlier this frame from
                // last frame's scratch deltas. The buffer convention follows
                // Capsaicin (gi1.comp:2165) — .rgb stores radiance × .w, so
                // the per-sample mean is .rgb / .w. The first frame after a
                // cache clear sees .w == 0 across the board and falls
                // through to BRDF sampling, no spurious zero-radiance hit.
                float4 cellRadiance = PTHashGridCache_UnpackRadiance(g_HashGridCache_ValueBuffer[cell_index]);
                if (cellRadiance.w > 0.0f)
                {
                    float3 mean = cellRadiance.rgb / cellRadiance.w;

                    // (b) PREVIOUS-vertex secondary-bounce contribution.
                    // Mirrors Capsaicin UpdateMultibounceCells (gi1.comp:
                    // 1962-1975). Skipped at bounce==1 (no prior secondary
                    // vertex) and when the prior iteration could not claim
                    // a cell. brdf_over_pdf reduces to throughput/prev_throughput
                    // because both factors share the path-prefix chain.
                    if (prev_cell_index != kPTHashGridCache_InvalidId)
                    {
                        // Component-wise safe divide: when a channel of
                        // prev_throughput collapsed to zero (e.g. albedo == 0
                        // on a surface), keep the contribution at zero rather
                        // than synthesising radiance out of a divide-by-zero.
                        float3 brdf_over_pdf = float3(
                            prev_throughput.x > 0.0f ? throughput.x / prev_throughput.x : 0.0f,
                            prev_throughput.y > 0.0f ? throughput.y / prev_throughput.y : 0.0f,
                            prev_throughput.z > 0.0f ? throughput.z / prev_throughput.z : 0.0f);
                        float3 secondaryContribution = brdf_over_pdf * mean;
                        uint4  quantizedSecondary    = PTHashGridCache_QuantizeRadiance(secondaryContribution);
                        InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * prev_cell_index + 0u], quantizedSecondary.x, prev_atomic);
                        InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * prev_cell_index + 1u], quantizedSecondary.y, prev_atomic);
                        InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * prev_cell_index + 2u], quantizedSecondary.z, prev_atomic);
                        InterlockedAdd(g_HashGridCache_UpdateCellValueBuffer[4u * prev_cell_index + 3u], quantizedSecondary.w, prev_atomic);
                    }

                    radiance += throughput * mean;
                    cacheTerminated = true;
                }

                // Carry forward this iteration's cell + throughput so the
                // NEXT iteration can attribute its cache-mean lookup back
                // here as a secondary-bounce contribution. Saved BEFORE the
                // BSDF importance sample updates throughput, so the next
                // iteration's `throughput / prev_throughput` recovers the
                // BRDF/pdf factor applied between vertices.
                prev_cell_index = cell_index;
                prev_throughput = throughput;
            }
        }

        if (cacheTerminated)
            break;
#endif

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
    float3 clampedRadiance = min(radiance, 100000.0f);
    AccumBuffer[pixel] = lerp(prev, float4(clampedRadiance, 1.0f), t);
}
