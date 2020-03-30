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
	float4 posCS : SV_POSITION;
	float3 posVS : POS_VS;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	float4 posWS = mul(input.position, perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posVS = posVS.xyz;
	output.posVS.z = log(output.posVS.z) / log(perFrameCBuffer.zFar - perFrameCBuffer.zNear);
	output.posCS = mul(posVS, perFrameCBuffer.p_original);
	output.TexCoord = input.texcoord;
	output.Normal = mul(input.normal, perObjectCBuffer.normalMat).xyz;

	return output;
}