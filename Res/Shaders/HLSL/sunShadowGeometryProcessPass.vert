// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	output.posWS = mul(float4(input.posLS, 1.0f), perObjectCBuffer.m);
	output.texCoord = input.texCoord;

	return output;
}