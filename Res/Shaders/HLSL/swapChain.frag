// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

[[vk::binding(0, 0)]]
Texture2D g_2DTexture : register(t0);

[[vk::binding(0, 1)]]
SamplerState g_Sampler : register(s0);

float4 main(PixelInputType input) : SV_TARGET
{
    float3 finalColor = g_2DTexture.Sample(g_Sampler, input.texCoord).xyz;

    return float4(finalColor, 1.0f);
}