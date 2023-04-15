// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float4 normalWS : NORMAL;
	float2 texCoord : TEXCOORD;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	output.posWS = mul(float4(input.posLS, 1.0f), perObjectCBuffer.m);
	output.normalWS = mul(float4(input.normalLS, 0.0f), perObjectCBuffer.normalMat);
	output.texCoord = input.texCoord;

	return output;
}