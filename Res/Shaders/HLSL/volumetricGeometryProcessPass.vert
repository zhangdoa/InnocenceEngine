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
	float4 posWS : POS_WS;
	float2 texcoord : TEXCOORD;
	float4 normal : NORMAL;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	float4 posWS = mul(input.position, perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS = mul(posVS, perFrameCBuffer.p_original);
	output.posWS = output.posCS / output.posCS.w;
	output.posWS.z = -posVS.z / (perFrameCBuffer.zFar - perFrameCBuffer.zNear);
	output.posWS.z = 1.0 - exp(-output.posWS.z * 8);
	output.texcoord = input.texcoord;
	output.normal = mul(input.normal, perObjectCBuffer.normalMat);

	return output;
}