// shadertype=hlsl

struct ShadowPayload
{
    bool isShadowed;
};

[shader("miss")]
void ShadowMissShader(inout ShadowPayload payload)
{
    payload.isShadowed = false;
}
