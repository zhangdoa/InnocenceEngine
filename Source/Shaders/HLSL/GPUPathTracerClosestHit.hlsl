// shadertype=hlsl
#include "common/common.hlsl"

struct PathTracerPayload
{
    float3 hitPos;
    float3 normal;
    float2 texCoord;
    float3 albedo;
    float  metalness;
    float  roughness;
    bool   missed;
};

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

[[vk::binding(2, 1)]]
StructuredBuffer<PTVertex> in_MegaVertexBuffer : register(t2);

[[vk::binding(3, 1)]]
StructuredBuffer<uint> in_MegaIndexBuffer : register(t3);

[[vk::binding(4, 1)]]
StructuredBuffer<MeshOffsetData> in_MeshOffsets : register(t4);

[[vk::binding(1, 1)]]
StructuredBuffer<MaterialCB> in_MaterialBuffer : register(t1);

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

    float2 uv0 = LoadVertexUV(offsets.vertexOffset, indices.x);
    float2 uv1 = LoadVertexUV(offsets.vertexOffset, indices.y);
    float2 uv2 = LoadVertexUV(offsets.vertexOffset, indices.z);
    payload.texCoord = uv0 * baryW + uv1 * barycentrics.x + uv2 * barycentrics.y;

    MaterialCB mat = in_MaterialBuffer[instanceID];
    // TODO(TASK-19): sample mat.TextureIndices[1] (albedo) / [0] (normal) / [2]
    // (metallic) / [3] (roughness) from a bindless Texture2D array once the
    // raytracing PSO gets a bindless SRV heap + sampler bound. For now the
    // payload carries the interpolated UV so the sample call is a one-line
    // addition once the bindings land.
    payload.albedo    = float3(mat.AlbedoR, mat.AlbedoG, mat.AlbedoB);
    payload.metalness = mat.Metallic;
    payload.roughness = mat.Roughness;
}
