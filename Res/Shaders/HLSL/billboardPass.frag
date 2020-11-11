// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 1)]]
Texture2D iconTexture : register(t0);

[[vk::binding(0, 2)]]
SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

struct PixelOutputType
{
	float4 billboardPass : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	float4 textureColor = iconTexture.Sample(SampleTypePoint, input.texcoord);
	if (textureColor.a == 0.0)
		discard;
	output.billboardPass = float4(textureColor.rgb, 1.0);

	return output;
}