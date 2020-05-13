// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION_ORIG;
	float4 posCS_prev : POSITION_PREV;
	nointerpolation float4 AABB : AABB;
	float2 texcoord : TEXCOORD;
};

RWTexture3D<float4> out_froxelizationPassRT0 : register(u0);
RWTexture3D<float4> out_froxelizationPassRT1 : register(u1);

void main(PixelInputType input)
{
	float3 writeCoord = input.posCS_orig.xyz;

	if (writeCoord.x < -1.0 || writeCoord.x > 1.0 || writeCoord.y < -1.0 || writeCoord.y > 1.0 || writeCoord.z < 0.0 || writeCoord.z > 1.0)
	{
		return;
	}
	else
	{
		writeCoord.xy = (writeCoord.xy * 0.5 + 0.5) * perFrameCBuffer.viewportSize / 8;
		writeCoord.z = input.posCS_orig.w;
		writeCoord.z *= 64;

		int3 writeCoordInt = int3(writeCoord);
		float3 motionVector = float3((input.posCS_orig.xy - input.posCS_prev.xy) * 0.5, input.posCS_orig.w - input.posCS_prev.w);

		// scattering RGB + extinction A
		out_froxelizationPassRT0[writeCoordInt] = materialCBuffer.albedo;
		// motion vector + phase function
		out_froxelizationPassRT1[writeCoordInt] = float4(motionVector, materialCBuffer.MRAT.x);
	}
}