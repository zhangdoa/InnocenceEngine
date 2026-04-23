// shadertype=hlsl

static const uint TILE_SIZE = 8;  // 8×8 probe tile size
static const uint SH_TILE_SIZE = 3;  // 3×3 SH storage per probe — 9 coefficients for bands 0–2 (GI-1.0 §2.4.2)

// Should be the same as the element count of the WorldProbeGrid buffer
static const uint HASH_TABLE_SIZE = 256 * 1024;
static const float3 probeSpacing = float3(0.125, 0.125, 0.125); // Adjust probe spacing as needed
static const uint2 probeAtlasSize = uint2(8, 8);
// GI-1.0 §2.1.1 temporal upscaling: one probe per (8*ξ_x, 8*ξ_y) spawn tile
// per frame, Halton-picked sub-pixel cycles through all probe-tile slots
// over ξ_x * ξ_y frames. (2,2) = quarter the ray budget per frame, full
// probe grid converges in 4 frames.
static const uint2 upscaleFactor = uint2(2, 2);
static const uint2 spawnTileSize = probeAtlasSize * upscaleFactor;

struct WorldProbe
{
    float3 positionWS;
    float3 radiance;        // Could later be SH coefficients
    float weight;           // Used for temporal accumulation
    uint fingerprint;       // [W.2] secondary hash of (pos, normal-octant, short-ray-bit); 0 = empty slot
};

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

// GI-1.0 §2.2 world-cache addressing — paper's "two distinct hashes that
// produce little to no collision between one another" (Jarzynski–Olano
// 2020). We use a PCG3D mix on the quantised grid index for the bucket
// and a different PCG3D constant set for the fingerprint, then linear-
// probe by fingerprint match in the consumers (RayGen / ClosestHit).
//
// Quantisation step is the same `probeSpacing` constant — different
// world positions inside the same cell intentionally collide so they
// share the cached outgoing radiance for that voxel.
//
// [W.2] descriptor is (quantised pos, octant(normal), short-ray bit) per
// paper §2.2 Fig. 14 — splits cache entries by outgoing direction so a
// floor (+Y normal) and ceiling (−Y normal) at the same voxel don't
// alias, and by ray length so near-surface AO (short bounces) is kept
// separate from distant-radiance (long bounces) contributions.
//
// READ / WRITE asymmetry: the WRITE side caches at the screen probe's
// world position using the probe's surface normal and the traced ray's
// travel distance. The READ side (ClosestHit, off-screen fallback) uses
// the hit position, `-WorldRayDirection()` as a normal proxy, and
// `RayTCurrent()` for the short-ray bit. Hits accept the cached value
// only when the hit-point's octant + short-ray classification matches
// what was written — exactly the leak-fix the paper targets.
static const float WORLD_PROBE_SHORT_RAY_THRESHOLD = 1.0;

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

uint3 _ProbeGridIndex(float3 positionWS)
{
    // Bias by 2^20 so negative world coordinates produce well-defined uints
    // before the PCG3D mix (avoids two's-complement aliasing in the bucket).
    int3 g = int3(floor(positionWS / probeSpacing));
    return uint3(g.x + (1 << 20), g.y + (1 << 20), g.z + (1 << 20));
}

// Octant-quantise a direction: sign bit per axis gives 8 bins. Coarse on
// purpose — the cache descriptor should collide across small normal
// variation within the same cell (smooth surfaces) and only split at
// axis crossings where the leak case actually lives.
uint3 _QuantizeNormalOctant(float3 n)
{
    return uint3(n.x >= 0.0 ? 1u : 0u,
                 n.y >= 0.0 ? 1u : 0u,
                 n.z >= 0.0 ? 1u : 0u);
}

bool IsShortRay(float distance)
{
    return distance < WORLD_PROBE_SHORT_RAY_THRESHOLD;
}

uint ComputeProbeHash(float3 positionWS, float3 normalWS, uint shortRayBit)
{
    uint3 posG = _ProbeGridIndex(positionWS);
    uint3 nG = _QuantizeNormalOctant(normalWS);
    // Sprinkle the direction/short-ray descriptor bits into high bits so
    // they perturb all three PCG3D output lanes without stomping the
    // low-bit grid-index entropy.
    uint3 mixed = posG ^ (nG << uint3(24u, 25u, 26u)) ^ uint3(shortRayBit * 0x5BD1E995u, 0, 0);
    uint3 v = _PCG3D(mixed);
    return (v.x ^ v.y ^ v.z) % HASH_TABLE_SIZE;
}

// Fingerprint reserves 0 as the "empty slot" sentinel so the linear-probe
// loop can distinguish "this slot was never written" from "wrong cell".
// Independent PCG3D constants from the bucket hash — two slots with the
// same bucket but different descriptors (position, normal octant,
// short-ray) are highly unlikely to share a fingerprint.
uint ComputeProbeFingerprint(float3 positionWS, float3 normalWS, uint shortRayBit)
{
    uint3 g = _ProbeGridIndex(positionWS);
    uint3 nG = _QuantizeNormalOctant(normalWS);
    uint3 shortMix = uint3(shortRayBit * 0x2E099Du, shortRayBit * 0x7F4A7C15u, shortRayBit * 0x9E3779B9u);
    uint3 seeded = g.zyx ^ (nG.zxy << uint3(27u, 28u, 29u)) ^ shortMix
                         ^ uint3(0xA341316Cu, 0xC8013EA4u, 0xAD90777Du);
    uint3 v = _PCG3D(seeded);
    uint fp = v.x ^ v.y ^ v.z;
    return fp == 0u ? 1u : fp;
}

// Linear probing — paper §2.2 ("linear probing inside the bucket"). Walk
// up to MAX_LINEAR_PROBE slots looking for a fingerprint match (lookup)
// or an empty / matching slot (insert). Returns HASH_TABLE_SIZE when no
// suitable slot is found; consumers must handle the miss.
static const uint MAX_LINEAR_PROBE = 8u;

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