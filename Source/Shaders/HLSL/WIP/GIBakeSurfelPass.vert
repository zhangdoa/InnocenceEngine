// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 normalWS : NORMAL;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	output.posWS = mul(float4(input.posLS, 1.0f), transformCBuffer.m);
	output.texCoord = input.texCoord;
	output.normalWS = mul(float4(input.normalLS, 0.0f), transformCBuffer.normalMat);

	return output;
}