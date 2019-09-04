// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 AABB : AABB;
};

struct PixelOutputType
{
	float4 volumetricPassRT0 : SV_Target0;
};

//RWTexture3D<float4> out_volumetricPassRT0 : register(u0);

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	float4 writeCoord = input.posCS;

	output.volumetricPassRT0 = writeCoord;

	return output;

	//writeCoord.xyz *= float3(160, 90, 64);

	//out_volumetricPassRT0[writeCoord.xyz] = float4(writeCoord.xyz, 1.0);
}