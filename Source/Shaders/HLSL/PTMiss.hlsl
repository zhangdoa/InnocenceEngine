// shadertype=hlsl
#include "common/common.hlsl"
#include "common/pathTracerPayload.hlsli"

[shader("miss")]
void MissShader(inout PTPayload payload)
{
    payload.missed = true;
}
