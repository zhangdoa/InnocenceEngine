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
	if (isnan(input.posCS_orig.z))
	{
		discard;
	}
	float4 posCS_orig = input.posCS_orig;
	posCS_orig.xy = posCS_orig.xy * 0.5 + 0.5;
	int3 writeCoord = int3((posCS_orig.xyz) * 64);

	out_froxelizationPassRT0[writeCoord] = float4(posCS_orig.xyz, 1.0f);
}