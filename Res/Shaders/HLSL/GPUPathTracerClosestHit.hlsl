// shadertype=hlsl
#include "common/common.hlsl"

// Mega geometry buffers (global bindings, set index 1)
[[vk::binding(2, 1)]]
ByteAddressBuffer in_MegaVertexBuffer : register(t2);   // PTVertex = {float3 pos, float3 normal}

[[vk::binding(3, 1)]]
ByteAddressBuffer in_MegaIndexBuffer  : register(t3);   // uint32 indices

// MeshOffsetBuffer: StructuredBuffer<uint2> where x=vertexOffset, y=indexOffset
[[vk::binding(4, 1)]]
ByteAddressBuffer in_MeshOffsets      : register(t4);

// MaterialConstantBuffer array (DrawCallService::GetMaterialBuffer())
[[vk::binding(1, 1)]]
StructuredBuffer<MaterialCB> in_MaterialBuffer : register(t1);

struct PathTracerPayload
{
    float3 hitPos;
    float3 normal;
    float3 albedo;
    float  metalness;
    float  roughness;
    bool   missed;
};

// MaterialCB matches MaterialConstantBuffer in GPUDataStructure.h
struct MaterialCB
{
    float AlbedoR, AlbedoG, AlbedoB, Alpha;
    float Metallic, Roughness, AO, Thickness;
    uint  TextureIndices[7];
    uint  MaterialType;
};

uint3 LoadTriangleIndices(uint baseIndex, uint primitiveIndex)
{
    uint byteOffset = (baseIndex + primitiveIndex * 3) * 4;
    return uint3(
        in_MegaIndexBuffer.Load(byteOffset + 0),
        in_MegaIndexBuffer.Load(byteOffset + 4),
        in_MegaIndexBuffer.Load(byteOffset + 8));
}

float3 LoadVertexNormal(uint baseVertex, uint vertexIndex)
{
    // PTVertex layout: {float3 pos (12B), float3 normal (12B)} = 24B per vertex
    uint byteOffset = (baseVertex + vertexIndex) * 24 + 12;  // skip pos
    float nx = asfloat(in_MegaVertexBuffer.Load(byteOffset + 0));
    float ny = asfloat(in_MegaVertexBuffer.Load(byteOffset + 4));
    float nz = asfloat(in_MegaVertexBuffer.Load(byteOffset + 8));
    return float3(nx, ny, nz);
}

[shader("closesthit")]
void ClosestHitShader(inout PathTracerPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint instanceID = InstanceID();  // = drawCallIndex after Task 4

    // Load mesh offsets for this instance
    uint2 offsets = uint2(
        in_MeshOffsets.Load(instanceID * 8 + 0),   // vertexOffset (bytes: instanceID*2 uints * 4B)
        in_MeshOffsets.Load(instanceID * 8 + 4));   // indexOffset

    // Interpolate normal
    uint3 tri = LoadTriangleIndices(offsets.y, PrimitiveIndex());
    float3 n0 = LoadVertexNormal(offsets.x, tri.x);
    float3 n1 = LoadVertexNormal(offsets.x, tri.y);
    float3 n2 = LoadVertexNormal(offsets.x, tri.z);

    float2 bary = attr.barycentrics;
    float3 localNormal = n0 * (1.0f - bary.x - bary.y) + n1 * bary.x + n2 * bary.y;
    payload.normal = normalize(mul((float3x3)ObjectToWorld3x4(), localNormal));
    payload.hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // Material lookup
    MaterialCB mat = in_MaterialBuffer[instanceID];
    payload.albedo    = float3(mat.AlbedoR, mat.AlbedoG, mat.AlbedoB);
    payload.metalness = mat.Metallic;
    payload.roughness = max(mat.Roughness, 0.04f);
    payload.missed    = false;
}
