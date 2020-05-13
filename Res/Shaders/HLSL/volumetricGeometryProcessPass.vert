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
	float4 posCS_orig : POSITION_ORIG;
	float4 posCS_prev : POSITION_PREV;
	float2 texcoord : TEXCOORD;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	float4 posWS = mul(input.position, perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS_orig = mul(posVS, perFrameCBuffer.p_original);
	output.posCS_orig /= output.posCS_orig.w;

	float4 posWS_prev = mul(input.position, perObjectCBuffer.m_prev);
	float4 posVS_prev = mul(posWS_prev, perFrameCBuffer.v_prev);
	output.posCS_prev = mul(posVS_prev, perFrameCBuffer.p_original);
	output.posCS_prev /= output.posCS_prev.w;

	output.posCS = mul(posVS, perFrameCBuffer.p_jittered);

	float expDepth = -posVS.z / (perFrameCBuffer.zFar - perFrameCBuffer.zNear);
	expDepth = 1.0 - exp(-expDepth * 8);

	float expDepth_prev = -posVS_prev.z / (perFrameCBuffer.zFar - perFrameCBuffer.zNear);
	expDepth_prev = 1.0 - exp(-expDepth_prev * 8);

	output.posCS_orig.w = expDepth;
	output.posCS_prev.w = expDepth_prev;

	output.texcoord = input.texcoord;

	return output;
}