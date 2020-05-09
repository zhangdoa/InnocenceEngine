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
	float3 posVS : POS_VS;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float4 posWS = mul(input.position, perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS = mul(posVS, perFrameCBuffer.p_original);

	output.posVS = output.posCS / output.posCS.w;
	output.posVS.z = -posVS.z / (perFrameCBuffer.zFar - perFrameCBuffer.zNear);
	output.posVS.z = 1.0 - exp(-output.posVS.z * 16);
	output.TexCoord = input.texcoord;
	output.Normal = mul(input.normal, perObjectCBuffer.normalMat).xyz;

	return output;
}