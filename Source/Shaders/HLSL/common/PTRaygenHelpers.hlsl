// shadertype=hlsl
#ifndef PT_RAYGEN_HELPERS_HLSL
#define PT_RAYGEN_HELPERS_HLSL

// Pure-function helpers for PTRayGen.hlsl: RNG (PCG / Halton),
// Disney/GGX BSDF math, hemisphere / GGX importance sampling, and the
// camera-ray + sky-shading helpers. Behaviour-preserving extraction from
// the raygen TU — every function body is unchanged from the pre-split
// inline form. Covered by the toggle=0 + toggle=1 DXIL byte-identity
// gate documented in the TASK-77.2 CL-1 implementation note.
//
// Prerequisite: the consumer must include common/skyResolver.hlsl and the
// per-frame CB declaration (common/PTRaygenBindings.hlsl) BEFORE including
// this header — `SkyColor` and `GenerateCameraRay` read `g_Frame`, and
// `getSkyColor` lives in skyResolver.hlsl.

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

#endif // PT_RAYGEN_HELPERS_HLSL
