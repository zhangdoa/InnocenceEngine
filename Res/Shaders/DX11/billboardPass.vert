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
	float2 texcoord : TEXCOORD;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float4 posWS = float4(m[3][0], m[3][1], m[3][2], 1.0);
	float distance = length(posWS - cam_globalPos);
	output.posCS = mul(posWS, cam_t);
	output.posCS = mul(output.posCS, cam_r);
	output.posCS = mul(output.posCS, cam_p_original);
	output.posCS /= output.posCS.w;
	float denom = distance;
	float2 shearingRatio = float2(1.0 / cam_WHRatio, 1.0) / clamp(denom, 1.0, distance);
	output.posCS.xy += input.position.xy * shearingRatio;
	output.texcoord = input.texcoord;

	return output;
}