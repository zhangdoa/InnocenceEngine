// shadertype=hlsl
#include "common/pathTracerPayload.hlsli"

[shader("miss")]
void ShadowMissShader(inout ShadowPayload payload)
{
    payload.isShadowed = false;
}
