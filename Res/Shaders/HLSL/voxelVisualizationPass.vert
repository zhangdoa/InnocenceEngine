// shadertype=hlsl
#include "common/common.hlsl"

struct VertexInputType
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	float2 pada : PADA;
	float4 normal : NORMAL;
	float4 padb : PADB;
	uint instanceId : SV_InstanceID;
};

struct GeometryInputType
{
	float4 posCS : SV_POSITION;
	float4 color : COLOR;
};

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