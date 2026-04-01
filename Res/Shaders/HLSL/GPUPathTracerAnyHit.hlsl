// shadertype=hlsl

struct PathTracerPayload
{
    float3 hitPos;
    float3 normal;
    float3 albedo;
    float  metalness;
    float  roughness;
    bool   missed;
};

[shader("anyhit")]
void AnyHitShader(inout PathTracerPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{

}
