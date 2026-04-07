// shadertype=hlsl
#include "common/common.hlsl"

struct PathTracerPayload
{
    float3 hitPos;
    float3 normal;
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

[shader("closesthit")]
void ClosestHitShader(inout PathTracerPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.missed = false;

    uint instanceID = InstanceID();
    MeshOffsetData offsets = in_MeshOffsets[instanceID];

    float2 barycentrics = attr.barycentrics;
    uint3 indices = LoadTriangleIndices(offsets.indexOffset, PrimitiveIndex());

    float3 n0 = LoadVertexNormal(offsets.vertexOffset, indices.x);
    float3 n1 = LoadVertexNormal(offsets.vertexOffset, indices.y);
    float3 n2 = LoadVertexNormal(offsets.vertexOffset, indices.z);
    float3 normal = n0 * (1.0f - barycentrics.x - barycentrics.y) + n1 * barycentrics.x + n2 * barycentrics.y;
    payload.normal = normalize(mul((float3x3)ObjectToWorld3x4(), normal));

    MaterialCB mat = in_MaterialBuffer[instanceID];
    payload.albedo    = float3(mat.AlbedoR, mat.AlbedoG, mat.AlbedoB);
    payload.metalness = mat.Metallic;
    payload.roughness = mat.Roughness;
}
