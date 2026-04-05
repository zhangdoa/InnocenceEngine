// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float3 posWS : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 normalWS : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float4 posWS = mul(float4(input.posLS, 1.0f), transformCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS = mul(posVS, perFrameCBuffer.p_original);

	output.posWS = posWS.xyz;
	output.TexCoord = input.texCoord;
	output.normalWS = mul(float4(input.normalLS, 0.0f), transformCBuffer.normalMat).xyz;

	return output;
}