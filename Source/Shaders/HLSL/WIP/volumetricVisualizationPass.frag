// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 1)]]
Texture3D in_volume : register(t0);
[[vk::binding(0, 2)]]
SamplerState in_samplerTypePoint : register(s0);

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

	float4 result = in_volume.Sample(in_samplerTypePoint, tc);

	output.visualizationPassRT0 = float4(result.xyz, 1.0);

	return output;
}