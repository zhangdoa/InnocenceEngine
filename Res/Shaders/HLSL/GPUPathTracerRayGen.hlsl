// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 0)]]
cbuffer PerFrameConstantBuffer : register(b0) { PerFrame_CB g_Frame; }

[[vk::binding(1, 0)]]
cbuffer FrameCountCB : register(b1) { uint g_FrameCount; }

[[vk::binding(0, 1)]]
RaytracingAccelerationStructure SceneAS : register(t0);

[[vk::binding(0, 2)]]
RWTexture2D<float4> AccumBuffer : register(u0);

struct PathTracerPayload
{
    float3 hitPos;
    float3 normal;
    float3 albedo;
    float  metalness;
    float  roughness;
    bool   missed;
};

struct ShadowPayload { bool isShadowed; };

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
    return a2 / (3.14159265f * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 CookTorranceGGX(float3 N, float3 V, float3 L, float3 albedo, float metalness, float roughness)
{
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
    float3 H  = normalize(V + L);

    float D  = DistributionGGX(N, H, roughness);
    float G  = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 specular = (D * G * F) / max(4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f), 0.001f);
    float3 diffuse  = (1.0f - F) * (1.0f - metalness) * albedo / 3.14159265f;

    return (diffuse + specular) * max(dot(N, L), 0.0f);
}

float3 ImportanceSampleGGX(float2 xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0f * 3.14159265f * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up    = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float3 SkyColor(float3 dir)
{
    return lerp(float3(0.1f, 0.15f, 0.2f), float3(0.5f, 0.7f, 1.0f), saturate(dir.y));
}

RayDesc GenerateCameraRay(uint2 pixel, float2 jitter, uint2 resolution)
{
    float2 uv = (float2(pixel) + 0.5f + jitter) / float2(resolution);
    uv.y = 1.0f - uv.y;
    float2 ndc = uv * 2.0f - 1.0f;

    float4 viewPos = mul(g_Frame.p_inv, float4(ndc.x, ndc.y, 1.0f, 1.0f));
    viewPos /= viewPos.w;
    float3 worldPos = mul(g_Frame.v_inv, float4(viewPos.xyz, 0.0f)).xyz;

    RayDesc ray;
    ray.Origin    = g_Frame.camera_posWS.xyz;
    ray.Direction = normalize(worldPos);
    ray.TMin      = 0.001f;
    ray.TMax      = 1e6f;
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

    for (int bounce = 0; bounce <= 40; ++bounce)
    {
        PathTracerPayload payload;
        payload.missed = false;

        TraceRay(SceneAS, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

        if (payload.missed)
        {
            radiance += throughput * SkyColor(ray.Direction);
            break;
        }

        float3 N  = payload.normal;
        float3 V  = -ray.Direction;
        float3 sunDir = normalize(g_Frame.sun_direction.xyz);

        // NEE: shadow ray toward sun
        ShadowPayload shadowPayload;
        shadowPayload.isShadowed = true;

        RayDesc shadowRay;
        shadowRay.Origin    = payload.hitPos + N * 0.001f;
        shadowRay.Direction = sunDir;
        shadowRay.TMin      = 0.001f;
        shadowRay.TMax      = 1e6f;

        TraceRay(SceneAS,
                 RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                 0xFF,
                 0,    // RayContributionToHitGroupIndex (unused — closest hit skipped)
                 1,
                 1,    // MissShaderIndex = 1 → ShadowMissShader
                 shadowRay, shadowPayload);

        if (!shadowPayload.isShadowed)
        {
            float3 brdf = CookTorranceGGX(N, V, sunDir, payload.albedo, payload.metalness, payload.roughness);
            radiance += throughput * brdf * g_Frame.sun_illuminance.xyz;
        }

        float2 xi = Rand2(rng);
        float3 H  = ImportanceSampleGGX(xi, N, payload.roughness);
        float3 L  = reflect(-V, H);

        float NdotL = dot(N, L);
        if (NdotL <= 0.0f)
            break;

        float3 brdfSample = CookTorranceGGX(N, V, L, payload.albedo, payload.metalness, payload.roughness);
        float  NdotH = max(dot(N, H), 0.0f);
        float  VdotH = max(dot(V, H), 0.0f);
        float  pdf   = DistributionGGX(N, H, payload.roughness) * NdotH / max(4.0f * VdotH, 0.001f);
        throughput *= brdfSample * NdotL / max(pdf, 0.001f);

        if (bounce >= 3)
        {
            float q = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05f, 0.95f);
            if (Rand(rng) > q)
                break;
            throughput /= q;
        }

        ray.Origin    = payload.hitPos + N * 0.001f;
        ray.Direction = L;
        ray.TMin      = 0.001f;
        ray.TMax      = 1e6f;
    }

    float w = 1.0f / float(g_FrameCount);
    float3 prev = AccumBuffer[pixel].rgb;
    AccumBuffer[pixel] = float4(lerp(prev, radiance, w), 1.0f);
}
