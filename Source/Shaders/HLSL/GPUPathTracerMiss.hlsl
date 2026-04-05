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

[shader("miss")]
void MissShader(inout PathTracerPayload payload)
{
    payload.missed = true;
}
