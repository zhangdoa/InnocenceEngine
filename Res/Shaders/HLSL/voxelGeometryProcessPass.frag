// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 1)]]
RWStructuredBuffer<uint> out_geometryProcessResult : register(u0);
[[vk::binding(0, 2)]]
Texture2D t2d_normal : register(t0);
[[vk::binding(1, 2)]]
Texture2D t2d_albedo : register(t1);
[[vk::binding(2, 2)]]
Texture2D t2d_metallic : register(t2);
[[vk::binding(3, 2)]]
Texture2D t2d_roughness : register(t3);
[[vk::binding(4, 2)]]
Texture2D t2d_ao : register(t4);
[[vk::binding(5, 2)]]
Texture2DArray in_SunShadow : register(t5);
[[vk::binding(0, 3)]]
SamplerState SamplerTypePoint : register(s0);

#include "common/shadowResolver.hlsl"

#include "common/BSDF.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION;
	float4 posWS : POS_WS;
	nointerpolation float4 AABB : AABB;
	float4 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

void main(PixelInputType input)
{
	//float3 pos = input.posCS.xyz;

	//pos *= voxelizationPassCBuffer.volumeResolutionRcp;

	//if (((pos.x < input.AABB.x) && (pos.y < input.AABB.y)) || ((pos.x > input.AABB.z) && (pos.y > input.AABB.w)))
	//{
	//	discard;
	//}

	float3 powWS = input.posCS_orig.xyz;

	if ((powWS.x < -1.0) || (powWS.y < -1.0) || (powWS.z < -1.0) || (powWS.x > 1.0) || (powWS.y > 1.0) || (powWS.z > 1.0))
	{
		discard;
	}

	float transparency = 1.0;
	float3 out_Albedo;
	if (materialCBuffer.textureSlotMask & 0x00000002)
	{
		float4 l_albedo = t2d_albedo.Sample(SamplerTypePoint, input.texcoord);
		transparency = l_albedo.a;
		if (transparency < 0.1)
		{
			discard;
		}
		else
		{
			out_Albedo = l_albedo.rgb;
		}
	}
	else
	{
		out_Albedo = materialCBuffer.albedo.rgb;
	}

	float out_Metallic;
	if (materialCBuffer.textureSlotMask & 0x00000004)
	{
		out_Metallic = t2d_metallic.Sample(SamplerTypePoint, input.texcoord).r;
	}
	else
	{
		out_Metallic = materialCBuffer.MRAT.r;
	}

	float out_Roughness;
	if (materialCBuffer.textureSlotMask & 0x00000008)
	{
		out_Roughness = t2d_roughness.Sample(SamplerTypePoint, input.texcoord).r;
	}
	else
	{
		out_Roughness = materialCBuffer.MRAT.g;
	}

	float out_AO;
	if (materialCBuffer.textureSlotMask & 0x00000010)
	{
		out_AO = t2d_ao.Sample(SamplerTypePoint, input.texcoord).r;
	}
	else
	{
		out_AO = materialCBuffer.MRAT.b;
	}

	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, out_Albedo, out_Metallic);

	float3 N = normalize(input.normal.xyz);
	float3 V = normalize(perFrameCBuffer.camera_posWS.xyz - input.posWS.xyz);
	float NdotV = max(dot(N, V), 0.0);

	float3 L = normalize(-perFrameCBuffer.sun_direction.xyz);
	float NdotL = max(dot(N, L), 0.0);

	float3 H = normalize(V + L);
	float LdotH = max(dot(L, H), 0.0);

	float F90 = 1.0;
	float3 FresnelFactor = fresnelSchlick(F0, F90, LdotH);
	float3 Ft = getBTDF(NdotV, NdotL, LdotH, out_Roughness, out_Metallic, FresnelFactor, out_Albedo);

	float3 illuminance = perFrameCBuffer.sun_illuminance.xyz * NdotL;
	float4 Lo = float4(illuminance * Ft, 1.0f);
	Lo *= 1.0 - SunShadowResolver(input.posWS.xyz, SamplerTypePoint);

	uint LoUint = EncodeColor(Lo);
	uint normalUint = EncodeNormal(input.normal);

	float3 writeCoord = (input.posCS_orig.xyz * 0.5 + 0.5) * voxelizationPassCBuffer.volumeResolution;
	int3 writeCoordInt = int3(writeCoord);
	int index = writeCoordInt.x + writeCoordInt.y * voxelizationPassCBuffer.volumeResolution + writeCoordInt.z * voxelizationPassCBuffer.volumeResolution * voxelizationPassCBuffer.volumeResolution;

	// @TODO: optimize
	int offset = voxelizationPassCBuffer.volumeResolution * voxelizationPassCBuffer.volumeResolution * voxelizationPassCBuffer.volumeResolution;

	InterlockedMax(out_geometryProcessResult[index], LoUint);
	InterlockedMax(out_geometryProcessResult[index + offset], normalUint);
}