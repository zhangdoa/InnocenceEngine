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
	writeCoord.xyz = writeCoord.xyz * 0.5 + 0.5;
	writeCoord.xyz = writeCoord.xyz * float3(160.0, 90.0, 64.0);
	out_volumetricPassRT0[writeCoord.xyz] = float4(writeCoord);
}