// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	uint rtvId : SV_RenderTargetArrayIndex;
};

struct PixelOutputType
{
	float4 shadowBlurRT0 : SV_Target0;
};

static const float2 gaussFilter[7] =
{
	-3.0,	0.015625,
	-2.0,	0.09375,
	-1.0,	0.234375,
	0.0,	0.3125,
	1.0,	0.234375,
	2.0,	0.09375,
	3.0,	0.015625
};

Texture2DArray in_shadowPassRT0 : register(t0);
SamplerState SamplerTypePoint : register(s0);

// ----------------------------------------------------------------------------
PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float2 renderTargetSize;
	float level;
	float elements;

	in_shadowPassRT0.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, elements, level);
	float2 texelSize = 1.0 / renderTargetSize;
	texelSize.y *= -1.0;

	float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for (int i = 0; i < 7; i++)
	{
		int3 coord = int3(input.texcoord * renderTargetSize + float2(gaussFilter[i].x, gaussFilter[i].x), input.rtvId);

		color += in_shadowPassRT0[coord] * gaussFilter[i].y;
	}

	output.shadowBlurRT0 = color;
	return output;
}