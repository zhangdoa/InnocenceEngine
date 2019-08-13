// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION;
	float UUID : ID;
	uint rtvId : SV_RenderTargetArrayIndex;
};

struct PixelOutputType
{
	float4 GIBrickFactorPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	output.GIBrickFactorPassRT0 = float4(input.posWS.xyz, input.UUID);

  return output;
}