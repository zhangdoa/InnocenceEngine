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

	output.sunShadowPass = float4(input.posCS.z, input.posCS.z * input.posCS.z, 0.0, 1.0);

	return output;
}