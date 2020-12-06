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

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.position = input.posLS;
	output.texCoord = input.texCoord;

	return output;
}