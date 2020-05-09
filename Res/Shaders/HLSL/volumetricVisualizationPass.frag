// shadertype=hlsl
#include "common/common.hlsl"

Texture3D in_volume : register(t0);

SamplerState SamplerTypePoint : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float3 posWS : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
};

struct PixelOutputType
{
	float4 visualizationPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	float3 tc = input.posCS.xyz;
	tc.xy /= perFrameCBuffer.viewportSize.xy;
	tc.xy = tc.xy * 0.5f + 0.5f;

	float4 result = in_volume.Sample(SamplerTypePoint, tc);

	output.visualizationPassRT0 = result;

	return output;
}