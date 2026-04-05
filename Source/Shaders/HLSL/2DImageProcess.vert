// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.position = float4(input.posLS, 1.0f);
	output.texCoord = input.texCoord;

	return output;
}