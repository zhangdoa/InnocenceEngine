// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
};

struct PixelOutputType
{
	float4 voxelVisualizationPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	if (input.posCS.a == 0.0)
	{
		discard;
	}

	output.voxelVisualizationPassRT0 = input.posCS;

	return output;
}