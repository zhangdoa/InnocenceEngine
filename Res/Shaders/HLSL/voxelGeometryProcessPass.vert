// shadertype=hlsl
#include "common/common.hlsl"

struct VertexInputType
{
	float4 posLS : POSITION;
	float2 texCoord : TEXCOORD;
	float2 pada : PADA;
	float4 normalLS : NORMAL;
	float4 padb : PADB;
};

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float4 normalWS : NORMAL;
	float2 texCoord : TEXCOORD;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	output.posWS = mul(input.posLS, perObjectCBuffer.m);
	output.normalWS = mul(input.normalLS, perObjectCBuffer.normalMat);
	output.texCoord = input.texCoord;

	return output;
}