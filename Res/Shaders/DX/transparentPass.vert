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
	float3 posWS : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float4 posWS = mul(input.position, meshCBuffer.m);
	float4 posVS = mul(posWS, cameraCBuffer.t);
	posVS = mul(posVS, cameraCBuffer.r);
	output.posCS = mul(posVS, cameraCBuffer.p_original);

	output.posWS = posWS.xyz;
	output.TexCoord = input.texcoord;
	output.Normal = mul(input.normal, meshCBuffer.normalMat).xyz;

	return output;
}