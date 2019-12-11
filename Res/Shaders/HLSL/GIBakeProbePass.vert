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

	output.posWS = mul(input.position, perObjectCBuffer.m);
	output.posCS = mul(output.posWS, GICBuffer.t);
	output.posCS = mul(output.posCS, GICBuffer.r[0]);
	output.posCS = mul(output.posCS, GICBuffer.p);
	output.posCS.z = output.posCS.z * 0.5 + 0.5;

	return output;
}