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
	float3 pos = input.posCS_orig.xyz;

	if (((pos.x < input.AABB.x) && (pos.y < input.AABB.y)) || ((pos.x > input.AABB.z) && (pos.y > input.AABB.w)))
	{
		discard;
	}

	out_voxelizationPassRT0[input.posCS_orig.xyz * 64] = input.posCS_orig;
}