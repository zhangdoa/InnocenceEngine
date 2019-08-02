// shadertype=hlsl
#include "comon/common.hlsl"

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
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.position = input.position;
	output.texcoord = input.texcoord;

	return output;
}