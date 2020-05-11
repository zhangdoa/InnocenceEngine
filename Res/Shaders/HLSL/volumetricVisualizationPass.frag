// shadertype=hlsl
#include "common/common.hlsl"

Texture3D in_volume : register(t0);

SamplerState SamplerTypePoint : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
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

	float3 tc = float3(input.posCS.xy / perFrameCBuffer.viewportSize.xy, 0.0f);
	tc = tc * 0.5f + 0.5f;
	tc.z = input.posCS.z;

	float4 result = in_volume.Sample(SamplerTypePoint, tc);

	output.visualizationPassRT0 = float4(result.xyz, 1.0);

	return output;
}