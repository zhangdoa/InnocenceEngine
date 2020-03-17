// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
};

struct PixelOutputType
{
	float4 froxelVisualizationPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	output.froxelVisualizationPassRT0 = input.posCS;

	return output;
}