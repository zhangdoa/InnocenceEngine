// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 1)]]
Texture2D in_BRDFLUT : register(t0);
[[vk::binding(1, 1)]]
Texture2D in_BRDFMSLUT : register(t1);
[[vk::binding(0, 2)]]
SamplerState in_samplerTypePoint : register(s0);

#include "common/BSDF.hlsl"

struct PixelInputType
{
	float4 frag_ClipSpacePos : SV_POSITION;
	float4 frag_ClipSpacePos_orig : POSITION_ORIG;
	float4 frag_ClipSpacePos_prev : POSITION_PREV;
	float3 frag_WorldSpacePos : POSITION;
	float2 frag_TexCoord : TEXCOORD;
	float3 frag_Normal : NORMAL;
};

struct PixelOutputType
{
	float4 RT : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	float3 N = input.frag_Normal;
	float3 V = normalize(perFrameCBuffer.camera_posWS.xyz - input.frag_WorldSpacePos);
	float3 L = N;
	float3 H = normalize(V + L);

	float LdotH = max(dot(L, H), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);

	float out_roughness = materialCBuffer.MRAT.g;
	out_roughness = (out_roughness + 0.05) / (1.05);
	float out_metallic = materialCBuffer.MRAT.r;
	float3 F0 = float3(0.04, 0.04, 0.04);
	float3 out_albedo = materialCBuffer.albedo.rgb;
	float3 luminance = float3(1.0, 1.0, 1.0);
	F0 = lerp(F0, out_albedo, out_metallic);

	float F90 = 1.0;
	float3 F = fresnelSchlick(F0, F90, LdotH);
	float G = V_SmithGGXCorrelated(NdotV, NdotL, out_roughness);
	float D = D_GGX(NdotH, out_roughness);
	float3 Frss = F * G * D;

	float3 Frms = getFrMS(in_BRDFLUT, in_BRDFMSLUT, in_samplerTypePoint, NdotL, NdotV, F0, out_roughness);

	float3 Fr = Frss + Frms;

	float3 kS = F;
	float3 kD = float3(1.0, 1.0, 1.0) - kS;

	kD *= 1.0 - out_metallic;

	float3 Fd = DisneyDiffuse2015(NdotV, NdotL, LdotH, out_roughness * out_roughness) * out_albedo;

	float3 Lo = (Fd * kD + Fr);

	output.RT = float4(Lo, 1.0f);

	return output;
}