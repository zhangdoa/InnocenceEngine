// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION;
	float4 AABB : AABB;
};

RWTexture3D<float4> out_froxelizationPassRT0 : register(u0);

void main(PixelInputType input)
{
	float4 posCS_orig = input.posCS_orig;
	posCS_orig.z = (2 * perFrameCBuffer.zNear) / (perFrameCBuffer.zFar + perFrameCBuffer.zNear - posCS_orig.z * (perFrameCBuffer.zFar - perFrameCBuffer.zNear));
	int3 writeCoord = int3((posCS_orig.xyz * 0.5 + 0.5) * 64);

	out_froxelizationPassRT0[writeCoord] = float4(1.0f, 1.0f, 1.0f, 1.0f);
}