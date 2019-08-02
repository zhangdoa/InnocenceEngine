// shadertype=hlsl
#include "common/common.hlsl"

Texture2D in_lightPassRT0 : register(t0);
Texture2D in_skyPassRT0 : register(t1);

SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 preTAAPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float2 flipYTexCoord = input.texcoord;
	flipYTexCoord.y = 1.0 - flipYTexCoord.y;

	float4 lightPassRT0 = in_lightPassRT0.Sample(SampleTypePoint, input.texcoord);
	float4 skyPassRT0 = in_skyPassRT0.Sample(SampleTypePoint, flipYTexCoord);

	float3 finalColor = float3(0.0, 0.0, 0.0);

	if (lightPassRT0.a == 0.0)
	{
		finalColor += skyPassRT0.rgb;
	}
	else
	{
		finalColor += lightPassRT0.rgb;
	}

	output.preTAAPassRT0 = float4(finalColor, 1.0);

	return output;
}