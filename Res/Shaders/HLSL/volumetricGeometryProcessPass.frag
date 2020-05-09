// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION;
	float4 posWS : POS_WS;
	nointerpolation float4 AABB : AABB;
	float4 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

RWTexture3D<float4> out_froxelizationPassRT0 : register(u0);

void main(PixelInputType input)
{
	float3 writeCoord = input.posCS_orig.xyz;
	writeCoord.xy = (writeCoord.xy * 0.5 + 0.5) * 64;
	writeCoord.z *= 64;

	int3 writeCoordInt = int3(writeCoord);
	float transparency = 1.0;

	float3 out_Albedo;
	out_Albedo = materialCBuffer.albedo.rgb;

	out_froxelizationPassRT0[writeCoordInt] = float4(out_Albedo, 1.0f);
}