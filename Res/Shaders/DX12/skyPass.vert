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
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float4 pos = float4(-1.0 * input.position.xyz, 1.0);
	pos = mul(pos, cameraCBuffer.r);
	pos = mul(pos, cameraCBuffer.p_original);
	pos.z = pos.w;
	output.frag_ClipSpacePos = pos;

	return output;
}