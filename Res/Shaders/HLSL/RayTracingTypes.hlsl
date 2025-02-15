// shadertype=hlsl

static const uint TILE_SIZE = 8;  // 8×8 probe tile size
static const uint SH_TILE_SIZE = 2;  // 2×2 SH storage per probe

// Should be the same as the element count of the WorldProbeGrid buffer
static const uint HASH_TABLE_SIZE = 256 * 1024;
static const float3 probeSpacing = float3(0.125, 0.125, 0.125); // Adjust probe spacing as needed
static const uint2 probeAtlasSize = uint2(8, 8);
static const uint2 upscaleFactor = uint2(1, 1); // No upscaling for now

struct WorldProbe
{
    float3 positionWS;
    float3 radiance; // Could later be SH coefficients
    float weight; // Used for temporal accumulation
};

// Payload structure passed between TraceRay calls.
struct RayPayload
{
    float3 radiance;
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

uint ComputeProbeHash(float3 positionWS)
{

    int3 gridIndex = int3(floor(positionWS / probeSpacing));

    uint hashKey = 1664525 * gridIndex.x + 1013904223;
    hashKey ^= 1664525 * gridIndex.y + 1013904223;
    hashKey ^= 1664525 * gridIndex.z + 1013904223;

    return hashKey % HASH_TABLE_SIZE;
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

// SH Basis Functions (first 3 bands)
float Y_00() { return 0.282095f; }  // L0 (DC)
float Y_1_1(float3 w) { return 0.488603f * w.y; }  // L1, -1 (Y)
float Y_10(float3 w) { return 0.488603f * w.z; }  // L1, 0 (Z)
float Y_11(float3 w) { return 0.488603f * w.x; }  // L1, 1 (X)