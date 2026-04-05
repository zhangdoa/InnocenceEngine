// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION;
};

struct PixelOutputType
{
	float4 GIProbePassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	output.GIProbePassRT0 = float4(input.posWS);

  return output;
}