// shadertype=hlsl
#include "common/common.hlsl"
#include "common/skyResolver.hlsl"
#include "common/pathTracerPayload.hlsli"

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0) { PerFrame_CB g_Frame; }

[[vk::binding(1, 0)]]
cbuffer FrameCountCB : register(b1) { uint g_FrameCount; }

[[vk::binding(2, 0)]]
cbuffer LightCountCB : register(b2) { uint g_PointLightCount; uint g_SphereLightCount; uint g_LightCountPad0; uint g_LightCountPad1; }

[[vk::binding(0, 1)]]
RaytracingAccelerationStructure SceneAS : register(t0);

[[vk::binding(5, 1)]]
StructuredBuffer<PointLight_CB> g_PointLights : register(t5);

[[vk::binding(6, 1)]]
StructuredBuffer<SphereLight_CB> g_SphereLights : register(t6);

[[vk::binding(0, 2)]]
RWTexture2D<float4> AccumBuffer : register(u0);

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

float3 SampleSunDirection(float3 sunDir, float2 xi)
{
    float r = sin(SUN_ANGULAR_RADIUS);
    float d = cos(SUN_ANGULAR_RADIUS);

    float phi = TWO_PI * xi.x;
    float cosTheta = 1.0f - xi.y * (1.0f - d);
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up    = abs(sunDir.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent   = normalize(cross(up, sunDir));
    float3 bitangent = cross(sunDir, tangent);

    return normalize(tangent * H.x + bitangent * H.y + sunDir * H.z);
}

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

    const uint MAX_BOUNCES = 4;

    for (uint bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        PathTracerPayload payload = (PathTracerPayload)0;
        payload.missed = true;

        TraceRay(SceneAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);

        if (payload.missed)
        {
            radiance += throughput * SkyColor(ray.Direction);
            break;
        }

        float3 N = normalize(payload.normal);
        float3 V = -ray.Direction;

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
            }
        }

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
