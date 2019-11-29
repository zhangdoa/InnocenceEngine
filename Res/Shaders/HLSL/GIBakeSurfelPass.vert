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

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float4 normal : NORMAL;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	output.posWS = mul(input.position, perObjectCBuffer.m);
	output.texcoord = input.texcoord;
	output.normal = mul(input.normal, perObjectCBuffer.normalMat);

	return output;
}