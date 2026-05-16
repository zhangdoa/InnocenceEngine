// shadertype=hlsl
#ifndef RAY_TRACING_TYPES_HLSL
#define RAY_TRACING_TYPES_HLSL

static const uint TILE_SIZE = 8;  // 8×8 probe tile size
static const uint SH_TILE_SIZE = 3;  // 3×3 SH storage per probe — 9 coefficients for bands 0–2 (GI-1.0 §2.4.2)

static const uint2 probeAtlasSize = uint2(8, 8);
// GI-1.0 §2.1.1 temporal upscaling: one probe per (8*ξ_x, 8*ξ_y) spawn tile
// per frame, Halton-picked sub-pixel cycles through all probe-tile slots
// over ξ_x * ξ_y frames. (2,2) = quarter the ray budget per frame, full
// probe grid converges in 4 frames.
static const uint2 upscaleFactor = uint2(2, 2);
static const uint2 spawnTileSize = probeAtlasSize * upscaleFactor;

// Payload structure passed between TraceRay calls.
// distance = ray-parameter t at the hit (or ray.TMax for a sky miss),
// used by screen-probe ray guiding to apply parallax correction when
// the reprojected sample originated from a spatially offset probe
// (paper §2.1.3, Figure 6).
struct RayPayload
{
    float3 radiance;
    float distance;
};

// Create tangent space for importance sampling
float3x3 CreateTangentSpace(float3 normal)
{
    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    return float3x3(tangent, bitangent, normal);
}

// Importance sampling GGX for indirect reflection rays
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float alpha = roughness * roughness;
    float phi = 2.0 * 3.14159265 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    float3x3 basis = CreateTangentSpace(N);
    return normalize(mul(H, basis));
}

// Random hash function for ray variation
float2 Hash2D(uint2 pixelID, uint seed)
{
    uint n = pixelID.x * 73856093u ^ pixelID.y * 19349663u ^ seed * 83492791u;
    n = (n << 13u) ^ n;
    return float2(
        (n * (n * n * 15731u + 789221u) + 1376312589u) & 0x7fffffff,
        (n * (n * n * 12347u + 45679u) + 987654321u) & 0x7fffffff
    ) / float(0x7fffffff);
}

// PCG3D hash (Jarzynski–Olano 2020). Used by closest-hit shaders for
// per-hit RNG seeding.
uint3 _PCG3D(uint3 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}

float2 EncodeOctahedral(float3 N)
{
    // Normalize using the sum of absolute components
    N /= (abs(N.x) + abs(N.y) + abs(N.z));

    // Fold negative Z into the XY plane
    if (N.z < 0.0f)
    {
        float2 oldXY = N.xy;
        N.x = (1.0f - abs(oldXY.y)) * sign(oldXY.x);
        N.y = (1.0f - abs(oldXY.x)) * sign(oldXY.y);
    }

    // Remap from [-1,1] to [0,1]
    return N.xy * 0.5f + 0.5f;
}

float3 DecodeOctahedral(float2 encoded)
{
    // Remap from [0,1] back to [-1,1]
    float2 f = encoded * 2.0f - 1.0f;

    // Recover partial normal
    float3 N = float3(f, 1.0f - abs(f.x) - abs(f.y));

    // Unfold if z < 0
    if (N.z < 0.0f)
    {
        float2 oldF = f;
        N.x = (1.0f - abs(oldF.y)) * sign(oldF.x);
        N.y = (1.0f - abs(oldF.x)) * sign(oldF.y);
    }

    // Normalize
    return normalize(N);
}

float2 GetAtlasTextureCoordinates(float2 screenCoordXY, float3 normalWS)
{
    float2 octUV = EncodeOctahedral(normalWS);
    return screenCoordXY + octUV * probeAtlasSize;
}

// SH Basis Functions, real form, bands 0–2 (9 coefficients).
// Paper §2.4.2 caps SH projection at 3 bands — irradiance integration
// against a cosine lobe has an analytic closed form per band, and higher
// bands are dominated by ray-tracing noise so their inclusion costs more
// variance than it gains detail.
float Y_00()          { return 0.282094791773878f; }
float Y_1_1(float3 w) { return 0.488602511902919f * w.y; }
float Y_10 (float3 w) { return 0.488602511902919f * w.z; }
float Y_11 (float3 w) { return 0.488602511902919f * w.x; }
float Y_2_2(float3 w) { return 1.092548430592079f * w.x * w.y; }
float Y_2_1(float3 w) { return 1.092548430592079f * w.y * w.z; }
float Y_20 (float3 w) { return 0.315391565252520f * (3.0f * w.z * w.z - 1.0f); }
float Y_21 (float3 w) { return 1.092548430592079f * w.x * w.z; }
float Y_22 (float3 w) { return 0.546274215296039f * (w.x * w.x - w.y * w.y); }

#endif // RAY_TRACING_TYPES_HLSL