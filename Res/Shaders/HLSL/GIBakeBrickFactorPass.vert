// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float UUID : ID;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	output.posWS = mul(float4(input.posLS, 1.0f), perObjectCBuffer.m);
	output.UUID = perObjectCBuffer.UUID;

	return output;
}