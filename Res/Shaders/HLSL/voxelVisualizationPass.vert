// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posCS : SV_POSITION;
	float4 color : COLOR;
};

[[vk::binding(0, 1)]]
Texture3D<float4> in_volume : register(t0);

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	float3 position = float3
	(
		input.instanceId % (int)voxelizationPassCBuffer.volumeResolution,
		(input.instanceId / (int)voxelizationPassCBuffer.volumeResolution) % (int)voxelizationPassCBuffer.volumeResolution,
		input.instanceId / ((int)voxelizationPassCBuffer.volumeResolution * (int)voxelizationPassCBuffer.volumeResolution)
	);

	int3 texPos = int3(position);
	output.posCS = float4(position * voxelizationPassCBuffer.volumeResolutionRcp, 1.0f);
	output.posCS.xyz = output.posCS.xyz * 2.0 - 1.0;
	output.color = in_volume[texPos];
	return output;
}