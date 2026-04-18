// shadertype=hlsl
#include "common/common.hlsl"
#include "common/pathTracerPayload.hlsli"

// Matches GPUPathTracerVertex in GPUPathTracerPass.h
struct PTVertex
{
    float posX, posY, posZ;
    float normX, normY, normZ;
    float texU, texV;
};

// Matches MeshOffsetData in GPUPathTracerPass.h
struct MeshOffsetData
{
    uint vertexOffset;
    uint indexOffset;
    uint vertexCount;
    uint indexCount;
};

// Matches MaterialConstantBuffer in GPUDataStructure.h
struct MaterialCB
{
    float AlbedoR, AlbedoG, AlbedoB, Alpha;
    float Metallic, Roughness, AO, Thickness;
    uint  TextureIndices[7];
    uint  MaterialType;
};

// TextureIndices slot conventions (match the rasterized opaque pass).
#define PT_TEX_SLOT_NORMAL    0
#define PT_TEX_SLOT_ALBEDO    1
#define PT_TEX_SLOT_METALLIC  2
#define PT_TEX_SLOT_ROUGHNESS 3
#define PT_INVALID_TEXTURE_INDEX 0xFFFFFFFF

[[vk::binding(2, 1)]]
StructuredBuffer<PTVertex> in_MegaVertexBuffer : register(t2);

[[vk::binding(3, 1)]]
StructuredBuffer<uint> in_MegaIndexBuffer : register(t3);

[[vk::binding(4, 1)]]
StructuredBuffer<MeshOffsetData> in_MeshOffsets : register(t4);

[[vk::binding(1, 1)]]
StructuredBuffer<MaterialCB> in_MaterialBuffer : register(t1);

// Bindless material-texture heap. Index with mat.TextureIndices[slot];
// PT_INVALID_TEXTURE_INDEX means "fall back to the CB scalar". Mirrors
// g_2DTextures in opaqueGeometryProcessPass.frag so a texture index is
// interchangeable between the rasterizer and the path tracer.
[[vk::binding(7, 1)]]
Texture2D g_MaterialTextures[] : register(t7);

[[vk::binding(0, 3)]]
SamplerState g_MaterialSampler : register(s0);

uint3 LoadTriangleIndices(uint baseIndex, uint primitiveIndex)
{
    uint i0 = baseIndex + primitiveIndex * 3;
    return uint3(
        in_MegaIndexBuffer[i0 + 0],
        in_MegaIndexBuffer[i0 + 1],
        in_MegaIndexBuffer[i0 + 2]);
}

float3 LoadVertexNormal(uint baseVertex, uint vertexIndex)
{
    PTVertex v = in_MegaVertexBuffer[baseVertex + vertexIndex];
    return float3(v.normX, v.normY, v.normZ);
}

float2 LoadVertexUV(uint baseVertex, uint vertexIndex)
{
    PTVertex v = in_MegaVertexBuffer[baseVertex + vertexIndex];
    return float2(v.texU, v.texV);
}

[shader("closesthit")]
void ClosestHitShader(inout PathTracerPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.missed = false;

    uint instanceID = InstanceID();
    MeshOffsetData offsets = in_MeshOffsets[instanceID];

    float2 barycentrics = attr.barycentrics;
    float baryW = 1.0f - barycentrics.x - barycentrics.y;
    uint3 indices = LoadTriangleIndices(offsets.indexOffset, PrimitiveIndex());

    float3 n0 = LoadVertexNormal(offsets.vertexOffset, indices.x);
    float3 n1 = LoadVertexNormal(offsets.vertexOffset, indices.y);
    float3 n2 = LoadVertexNormal(offsets.vertexOffset, indices.z);
    float3 normal = n0 * baryW + n1 * barycentrics.x + n2 * barycentrics.y;
    payload.normal = normalize(mul((float3x3)ObjectToWorld3x4(), normal));

    // Two-sided shading for thin / single-faced geometry (Sponza curtains,
    // cloth, leaves). When the ray hits the back of a triangle, the
    // interpolated vertex normal points into the incoming hemisphere — BRDF
    // eval then sees cos(N, V) < 0 and the Fresnel term goes haywire, which
    // made curtains look like chrome under the path tracer (TASK-69). Flip
    // the shading normal so the BRDF always sees a ray-facing surface. The
    // geometric side is conceptually identical for thin fabric; for truly
    // solid objects, back-face hits should be vanishingly rare.
    if (dot(payload.normal, WorldRayDirection()) > 0.0f)
        payload.normal = -payload.normal;

    float2 uv0 = LoadVertexUV(offsets.vertexOffset, indices.x);
    float2 uv1 = LoadVertexUV(offsets.vertexOffset, indices.y);
    float2 uv2 = LoadVertexUV(offsets.vertexOffset, indices.z);
    payload.texCoord = uv0 * baryW + uv1 * barycentrics.x + uv2 * barycentrics.y;

    MaterialCB mat = in_MaterialBuffer[instanceID];

    // Scalar CB values are the fallback; bound-texture sample overrides per-slot.
    float3 albedo    = float3(mat.AlbedoR, mat.AlbedoG, mat.AlbedoB);
    float  metalness = mat.Metallic;
    float  roughness = mat.Roughness;

    // SampleLevel(0) because gradients aren't defined in ray-tracing shaders
    // (no 2x2 quad neighborhood → can't derive mip level from ddx/ddy).
    uint albedoIdx = mat.TextureIndices[PT_TEX_SLOT_ALBEDO];
    if (albedoIdx != PT_INVALID_TEXTURE_INDEX)
        albedo = g_MaterialTextures[NonUniformResourceIndex(albedoIdx)].SampleLevel(g_MaterialSampler, payload.texCoord, 0.0f).rgb;

    // glTF packs metallic and roughness into one texture: R = unused/occlusion,
    // G = roughness, B = metalness (glTF 2.0 spec, section 5.22). Our
    // AssimpMaterialProcessor points slot 2 (METALLIC) at the same texture
    // as slot 3 (ROUGHNESS) because Assimp reports aiTextureType_METALNESS
    // and aiTextureType_DIFFUSE_ROUGHNESS as both referring to the packed
    // texture. Sample from the right channel for each slot. Sampling .r for
    // both (previous bug) read occlusion / zero and collapsed every glTF
    // surface to the scalar fallback "fully metallic, fully rough."
    uint metallicIdx = mat.TextureIndices[PT_TEX_SLOT_METALLIC];
    if (metallicIdx != PT_INVALID_TEXTURE_INDEX)
        metalness = g_MaterialTextures[NonUniformResourceIndex(metallicIdx)].SampleLevel(g_MaterialSampler, payload.texCoord, 0.0f).b;

    uint roughnessIdx = mat.TextureIndices[PT_TEX_SLOT_ROUGHNESS];
    if (roughnessIdx != PT_INVALID_TEXTURE_INDEX)
        roughness = g_MaterialTextures[NonUniformResourceIndex(roughnessIdx)].SampleLevel(g_MaterialSampler, payload.texCoord, 0.0f).g;

    payload.albedo    = albedo;
    payload.metalness = metalness;
    payload.roughness = roughness;
}
