// shadertype=hlsl
#include "common/common.hlsl"

Texture2D t2d_albedo : register(t0);

SamplerState SampleTypeWrap : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float2 texcoord : TEXCOORD;
	uint rtvId : SV_RenderTargetArrayIndex;
};

struct PixelOutputType
{
	float4 sunShadowPass : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	float depth = input.posCS.z;
	depth = depth * 0.5 + 0.5;
	depth = saturate(depth);

	float transparency;
	if (materialCBuffer.textureSlotMask & 0x00000002)
	{
		float4 l_albedo = t2d_albedo.Sample(SampleTypeWrap, input.texcoord);
		transparency = l_albedo.a;
	}
	else
	{
		transparency = materialCBuffer.albedo.a;
	}

	if (transparency == 0.0)
	{
		discard;
	}

	output.sunShadowPass = float4(depth, depth * depth, 0.0, 1.0);

	return output;
}