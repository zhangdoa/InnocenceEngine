// shadertype=hlsl
#include "common/common.hlsl"

struct VertexInputType
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	float2 pada : PADA;
	float4 normal : NORMAL;
	float4 padb : PADB;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.posWS = mul(input.position, meshCBuffer.m);
	output.posCS = mul(output.posWS, GICamera_t);
	output.posCS = mul(output.posCS, GICamera_r[0]);
	output.posCS = mul(output.posCS, GICamera_p);
	output.posCS.z = output.posCS.z * 0.5 + 0.5;

	return output;
}