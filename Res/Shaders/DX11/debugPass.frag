// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
};

struct PixelOutputType
{
	float4 debugPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	output.debugPassRT0 = float4(0.2, 0.3, 0.4, 1.0);

	return output;
}