// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION;
	float4 AABB : AABB;
};

RWTexture3D<float4> out_voxelizationPassRT0 : register(u0);

void main(PixelInputType input)
{
	//float3 pos = input.posCS.xyz;

	//pos /= voxelizationPassCBuffer.voxelResolution.xyz;

	//if (((pos.x < input.AABB.x) && (pos.y < input.AABB.y)) || ((pos.x > input.AABB.z) && (pos.y > input.AABB.w)))
	//{
	//	discard;
	//}

	int3 writeCoord = int3((input.posCS_orig.xyz * 0.5 + 0.5) * voxelizationPassCBuffer.voxelResolution.xyz);

	out_voxelizationPassRT0[writeCoord] = materialCBuffer.albedo;
}