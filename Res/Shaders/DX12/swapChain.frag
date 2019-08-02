// shadertype=hlsl
#include "comon/common.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D pipelineResult : register(t0);
SamplerState SamplerTypePoint : register(s0);

float4 main(PixelInputType input) : SV_TARGET
{
	float3 finalColor = pipelineResult.Sample(SamplerTypePoint, input.texcoord).xyz;

	return float4(finalColor, 1.0f);
}