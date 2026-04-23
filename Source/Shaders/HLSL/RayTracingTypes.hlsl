// shadertype=hlsl

static const uint TILE_SIZE = 8;  // 8×8 probe tile size
static const uint SH_TILE_SIZE = 3;  // 3×3 SH storage per probe — 9 coefficients for bands 0–2 (GI-1.0 §2.4.2)

// GI-1.0 §2.2.3 two-level tiled world cache. Top-level hash table stores
// WorldTile slots; each tile holds 64 MIP0 cells (8×8) + 16 MIP1 + 4 MIP2
// + 1 MIP3 = 85 cells total. Element count of the WorldTileGrid buffer
// must match WORLD_TILE_HASH_SIZE.
static const uint WORLD_TILE_HASH_SIZE = 32 * 1024;
static const float3 probeSpacing = float3(0.125, 0.125, 0.125); // World-cache cell size; tile extent is 8× this on each axis.
static const uint WORLD_TILE_CELLS_PER_AXIS = 8u;
// MIP chain offsets inside WorldTile.cells[]: MIP0 @0..63 (8×8), MIP1 @64..79 (4×4), MIP2 @80..83 (2×2), MIP3 @84 (1×1).
static const uint WORLD_TILE_CELLS_TOTAL = 85u;
static const uint2 probeAtlasSize = uint2(8, 8);
// GI-1.0 §2.1.1 temporal upscaling: one probe per (8*ξ_x, 8*ξ_y) spawn tile
// per frame, Halton-picked sub-pixel cycles through all probe-tile slots
// over ξ_x * ξ_y frames. (2,2) = quarter the ray budget per frame, full
// probe grid converges in 4 frames.
static const uint2 upscaleFactor = uint2(2, 2);
static const uint2 spawnTileSize = probeAtlasSize * upscaleFactor;

// One cell inside a WorldTile. weight == 0 flags "cell has never been written"
// so lookups can distinguish "cell valid" from "tile recently reclaimed and
// not yet repopulated at this coordinate".
struct WorldCell
{
    float3 radiance;
    float weight;
};

struct WorldTile
{
    uint fingerprint;       // secondary hash; 0 = empty slot
    uint lastTouchedFrame;  // g_Frame.frameIndex at last insert/update; drives decay-based eviction
    uint _pad0;
    uint _pad1;
    WorldCell cells[WORLD_TILE_CELLS_TOTAL];  // 85 × 16 B = 1360 B. Tile total = 1376 B.
};

// Decay-based eviction: a tile slot whose lastTouchedFrame is older than
// the current frame by more than this threshold is treated as empty for
// INSERT purposes (lookups still reject on fingerprint mismatch, so stale
// radiance isn't served). ~1 second at 60 fps — long enough that actively-
// refreshed tiles survive a brief ray-budget gap, short enough that a
// scene edit flushes within seconds.
static const uint WORLD_TILE_EVICTION_AGE = 64u;

bool IsTileSlotStale(uint slotLastTouchedFrame, uint currentFrame)
{
    // Unsigned subtraction wraps cleanly: a freshly-written tile reads
    // small, an uninitialised slot reads 0 so the first-ever frame
    // returns currentFrame which crosses the threshold naturally once
    // the frame counter exceeds it.
    return (currentFrame - slotLastTouchedFrame) > WORLD_TILE_EVICTION_AGE;
}

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

// GI-1.0 §2.2 world-cache addressing — two independent PCG3D hashes
// (Jarzynski–Olano 2020) for bucket + fingerprint, with open-addressing
// linear probing by fingerprint match in the consumers (RayGen / ClosestHit).
//
// §2.2.3 two-level tiled descriptor: hash is keyed by tile position, a
// 6-bin dominant-axis direction bin, and a short-ray bit. Cell position
// inside the tile is derived separately (CellInTile) and indexes into
// WorldTile.cells[]. The tile's 2D cell plane is orthogonal to the
// dominant axis of the outgoing direction — cells at different offsets
// along the dominant axis collapse into the same 2D cell and are
// averaged inside the MIP chain.
//
// READ / WRITE asymmetry: the WRITE side caches at the screen probe's
// world position using the probe's surface normal (dominant axis of
// the outgoing diffuse hemisphere) and the traced ray's travel distance.
// The READ side (ClosestHit, off-screen fallback) uses the hit position,
// `-WorldRayDirection()` as a dominant-direction proxy, and RayTCurrent()
// for the short-ray bit. Hits accept the cached value only when the
// hit-point's axis + short-ray classification matches what was written.
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

// Tile grid index — one step per (probeSpacing * cells-per-axis). Bias
// by 2^17 so negative world coordinates produce well-defined uints
// before the PCG3D mix.
uint3 _TileGridIndex(float3 positionWS)
{
    float3 tileExtent = probeSpacing * float(WORLD_TILE_CELLS_PER_AXIS);
    int3 g = int3(floor(positionWS / tileExtent));
    return uint3(g.x + (1 << 17), g.y + (1 << 17), g.z + (1 << 17));
}

// Cell grid index — one step per probeSpacing, for in-tile coord lookup.
// 2^20 bias matches what the legacy hash used.
uint3 _CellGridIndex(float3 positionWS)
{
    int3 g = int3(floor(positionWS / probeSpacing));
    return uint3(g.x + (1 << 20), g.y + (1 << 20), g.z + (1 << 20));
}

// 6-bin dominant-axis direction: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z.
// Paper §2.2.3: the tile's 2D cell plane is perpendicular to this axis,
// cells along the axis collapse into one 2D slot and average in MIP.
uint DominantAxis(float3 dir)
{
    float3 a = abs(dir);
    uint axis;
    if (a.x >= a.y && a.x >= a.z)
        axis = 0u;
    else if (a.y >= a.z)
        axis = 1u;
    else
        axis = 2u;
    uint signBit;
    if (axis == 0u)       signBit = dir.x >= 0.0 ? 0u : 1u;
    else if (axis == 1u)  signBit = dir.y >= 0.0 ? 0u : 1u;
    else                  signBit = dir.z >= 0.0 ? 0u : 1u;
    return axis * 2u + signBit;
}

// 2D cell coordinate inside a tile, given dominant axis. Picks the two
// axes orthogonal to the dominant axis; cells along the dominant axis
// all map to the same 2D slot.
uint2 CellInTile(float3 positionWS, uint dominantAxisBin)
{
    uint3 g = _CellGridIndex(positionWS);
    uint axis = dominantAxisBin >> 1u;
    if (axis == 0u) return uint2(g.y & 7u, g.z & 7u); // X-dominant → YZ cells
    if (axis == 1u) return uint2(g.x & 7u, g.z & 7u); // Y-dominant → XZ cells
    return uint2(g.x & 7u, g.y & 7u);                 // Z-dominant → XY cells
}

// MIP cell index inside WorldTile.cells[]. mipLevel 0 = 8×8, 1 = 4×4, 2 = 2×2, 3 = 1×1.
uint CellIndexInTile(uint2 cellXY, uint mipLevel)
{
    // MIP offsets: 0, 64, 80, 84.
    uint offset;
    if      (mipLevel == 0u) offset = 0u;
    else if (mipLevel == 1u) offset = 64u;
    else if (mipLevel == 2u) offset = 80u;
    else                     offset = 84u;
    uint axisLen = WORLD_TILE_CELLS_PER_AXIS >> mipLevel; // 8, 4, 2, 1
    return offset + cellXY.y * axisLen + cellXY.x;
}

// Distance-to-MIP selection for reads. Short rays — MIP0 (sharpest). As
// the traced distance grows the cone footprint widens; reading from a
// higher MIP prefilters many cells' contributions and reduces variance.
// Heuristic: 1 m → MIP0, 4 m → MIP1, 16 m → MIP2, 64 m+ → MIP3.
uint SelectTileMipLevel(float distance)
{
    float m = log2(max(distance, 1.0)) * 0.5 - 0.5;
    return uint(clamp(m, 0.0, 3.0));
}

bool IsShortRay(float distance)
{
    return distance < WORLD_PROBE_SHORT_RAY_THRESHOLD;
}

uint ComputeTileHash(float3 positionWS, float3 directionWS, uint shortRayBit)
{
    uint3 tposG = _TileGridIndex(positionWS);
    uint axisBin = DominantAxis(directionWS);
    // Sprinkle the axis/short-ray descriptor bits into high bits so they
    // perturb all three PCG3D output lanes without stomping the low-bit
    // tile-index entropy.
    uint3 mixed = tposG ^ uint3(axisBin * 0x5BD1E995u,
                                shortRayBit * 0x7F4A7C15u,
                                (axisBin ^ shortRayBit) * 0x165667B1u);
    uint3 v = _PCG3D(mixed);
    return (v.x ^ v.y ^ v.z) % WORLD_TILE_HASH_SIZE;
}

// Fingerprint reserves 0 as the "empty slot" sentinel so the linear-probe
// loop can distinguish "never written" from "different tile". Independent
// PCG3D constants from the bucket hash — two slots with the same bucket
// but different descriptors are highly unlikely to share a fingerprint.
uint ComputeTileFingerprint(float3 positionWS, float3 directionWS, uint shortRayBit)
{
    uint3 tposG = _TileGridIndex(positionWS);
    uint axisBin = DominantAxis(directionWS);
    uint3 shortMix = uint3(shortRayBit * 0x2E099Du, shortRayBit * 0x7F4A7C15u, shortRayBit * 0x9E3779B9u);
    uint3 seeded = tposG.zyx ^ uint3(axisBin * 0x27D4EB2Du,
                                     axisBin * 0x9E3779B1u,
                                     axisBin * 0x85EBCA77u)
                             ^ shortMix
                             ^ uint3(0xA341316Cu, 0xC8013EA4u, 0xAD90777Du);
    uint3 v = _PCG3D(seeded);
    uint fp = v.x ^ v.y ^ v.z;
    return fp == 0u ? 1u : fp;
}

// Linear probing — paper §2.2 ("linear probing inside the bucket"). Walk
// up to MAX_LINEAR_PROBE slots looking for a fingerprint match (lookup)
// or an empty / matching / stale slot (insert). Consumers handle the miss.
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