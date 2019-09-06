// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 AABB : AABB;
};

RWTexture3D<float4> out_volumetricPassRT0 : register(u0);

void main(PixelInputType input)
{
	float4 writeCoord = input.posCS;

	writeCoord.z *= 64;

	out_volumetricPassRT0[writeCoord.xyz] = float4(writeCoord.xyz / float3(160, 90, 64), 1.0);
}