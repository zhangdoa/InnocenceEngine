// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
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

	output.sunShadowPass = float4(depth, depth * depth, 0.0, 1.0);

	return output;
}