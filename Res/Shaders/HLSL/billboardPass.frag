// shadertype=hlsl
#include "common/common.hlsl"

Texture2D iconTexture : register(t0);
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

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float4 textureColor = iconTexture.Sample(SampleTypePoint, input.texcoord);
	if (textureColor.a == 0.0)
		discard;
	output.billboardPass = float4(textureColor.rgb, 1.0);

	return output;
}