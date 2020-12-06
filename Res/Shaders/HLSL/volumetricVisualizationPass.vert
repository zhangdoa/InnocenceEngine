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
	float4 posCS : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float4 posWS = mul(input.posLS, perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS = mul(posVS, perFrameCBuffer.p_original);
	output.TexCoord = input.texCoord;
	output.Normal = mul(input.normalLS, perObjectCBuffer.normalMat).xyz;

	return output;
}