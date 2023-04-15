// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float4 posWS = mul(float4(input.posLS, 1.0f), perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS = mul(posVS, perFrameCBuffer.p_original);
	output.TexCoord = input.texCoord;
	output.Normal = mul(float4(input.normalLS, 0.0f), perObjectCBuffer.normalMat).xyz;

	return output;
}