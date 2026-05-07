// shadertype=hlsl
#include "common/common.hlsl"
#include "common/pathTracerPayload.hlsli"

// Mirrors Inno::TVertex<float> in Math.h — pragma pack(1), 64 bytes.
// pad fields exist on the C++ side; the shader reads only pos/normal/texCoord.
struct MeshVertex
{
    float3 pos;
    float3 normal;
    float3 tangent;
    float2 texCoord;
    float  pad[5];
};

// Matches MaterialConstantBuffer in GPUDataStructure.h
struct MaterialCB
{
    float AlbedoR, AlbedoG, AlbedoB, Alpha;
    float Metallic, Roughness, AO, Thickness;
    uint  TextureIndices[7];
    uint  MaterialType;
};

#define PT_TEX_SLOT_NORMAL    0
#define PT_TEX_SLOT_ALBEDO    1
#define PT_TEX_SLOT_METALLIC  2
#define PT_TEX_SLOT_ROUGHNESS 3
#define PT_INVALID_TEXTURE_INDEX 0xFFFFFFFF

[[vk::binding(1, 1)]]
StructuredBuffer<MaterialCB> in_MaterialBuffer : register(t1);

[[vk::binding(7, 1)]]
Texture2D g_MaterialTextures[] : register(t7);

// Bindless per-mesh attribute arrays — engine auto-binds the dedicated
// mesh-attribute SRV heaps. Each lives in its own register space so the
// unbounded ranges don't collide with each other or with t7 g_MaterialTextures.
// Indexed by InstanceID() = mesh-asset SRV slot.
[[vk::binding(8, 1)]]
StructuredBuffer<MeshVertex> g_MeshVertexBuffers[] : register(t0, space1);

[[vk::binding(9, 1)]]
Buffer<uint> g_MeshIndexBuffers[] : register(t0, space2);

[[vk::binding(0, 3)]]
SamplerState g_MaterialSampler : register(s0);

[shader("closesthit")]
void ClosestHitShader(inout PathTracerPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.missed = false;
    payload.instanceID = InstanceID();

    uint meshSlot = InstanceID();
    uint primIdx = PrimitiveIndex();

    uint i0 = g_MeshIndexBuffers[NonUniformResourceIndex(meshSlot)][primIdx * 3 + 0];
    uint i1 = g_MeshIndexBuffers[NonUniformResourceIndex(meshSlot)][primIdx * 3 + 1];
    uint i2 = g_MeshIndexBuffers[NonUniformResourceIndex(meshSlot)][primIdx * 3 + 2];

    MeshVertex v0 = g_MeshVertexBuffers[NonUniformResourceIndex(meshSlot)][i0];
    MeshVertex v1 = g_MeshVertexBuffers[NonUniformResourceIndex(meshSlot)][i1];
    MeshVertex v2 = g_MeshVertexBuffers[NonUniformResourceIndex(meshSlot)][i2];

    float2 barycentrics = attr.barycentrics;
    float baryW = 1.0f - barycentrics.x - barycentrics.y;

    float3 normal = v0.normal * baryW + v1.normal * barycentrics.x + v2.normal * barycentrics.y;
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

    payload.texCoord = v0.texCoord * baryW + v1.texCoord * barycentrics.x + v2.texCoord * barycentrics.y;

    MaterialCB mat = in_MaterialBuffer[InstanceIndex()];

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
