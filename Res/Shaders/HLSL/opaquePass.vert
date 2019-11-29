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
	float4 frag_ClipSpacePos : SV_POSITION;
	float4 frag_ClipSpacePos_orig : POSITION_ORIG;
	float4 frag_ClipSpacePos_prev : POSITION_PREV;
	float3 frag_WorldSpacePos : POSITION;
	float2 frag_TexCoord : TEXCOORD;
	float3 frag_Normal : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float4 frag_WorldSpacePos = mul(input.position, perObjectCBuffer.m);
	float4 frag_CameraSpacePos = mul(frag_WorldSpacePos, perFrameCBuffer.v);
	output.frag_ClipSpacePos_orig = mul(frag_CameraSpacePos, perFrameCBuffer.p_original);

	float4 frag_WorldSpacePos_prev = mul(input.position, perObjectCBuffer.m_prev);
	float4 frag_CameraSpacePos_prev = mul(frag_WorldSpacePos_prev, perFrameCBuffer.v_prev);
	output.frag_ClipSpacePos_prev = mul(frag_CameraSpacePos_prev, perFrameCBuffer.p_original);

	output.frag_ClipSpacePos = mul(frag_CameraSpacePos, perFrameCBuffer.p_jittered);

	output.frag_WorldSpacePos = frag_WorldSpacePos.xyz;
	output.frag_TexCoord = input.texcoord;
	output.frag_Normal = mul(input.normal, perObjectCBuffer.normalMat).xyz;

	return output;
}