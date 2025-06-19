// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.posWS = mul(float4(input.posLS, 1.0f), transformCBuffer.m);
	output.posCS = mul(output.posWS, GICBuffer.t);
	output.posCS = mul(output.posCS, GICBuffer.r[0]);
	output.posCS = mul(output.posCS, GICBuffer.p);
	output.posCS.z = output.posCS.z * 0.5 + 0.5;

	return output;
}