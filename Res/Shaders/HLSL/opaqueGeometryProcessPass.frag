// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 0)]]
cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(0, 1)]]
StructuredBuffer<PerObject_CB> g_Objects : register(t0);

[[vk::binding(1, 1)]]
StructuredBuffer<Material_CB> g_Materials : register(t1);

[[vk::binding(2, 1)]]
Texture2D g_2DTextures[] : register(t2);

[[vk::binding(0, 2)]]
SamplerState g_Sampler : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION_ORIG;
	float4 posCS_prev : POSITION_PREV;
	float3 posWS : POSITION;
	float2 texCoord : TEXCOORD;
	float3 normalWS : NORMAL;
	float3 tangentWS : TANGENT;
};

struct PixelOutputType
{
	float4 opaquePassRT0 : SV_Target0;
	float4 opaquePassRT1 : SV_Target1;
	float4 opaquePassRT2 : SV_Target2;
	float4 opaquePassRT3 : SV_Target3;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	PerObject_CB perObjectCB = g_Objects[m_ObjectIndex];
	Material_CB materialCBuffer = g_Materials[perObjectCB.m_MaterialIndex];

	float3 normalWS = normalize(input.normalWS);
	int normalTextureIndex = materialCBuffer.m_TextureIndices_0;
	if (normalTextureIndex != -1)
	{
		float3 N = normalize(input.normalWS);
		float3 T = normalize(input.tangentWS);
		float3 B = cross(N, T);
		float3x3 TBN = float3x3(T, B, N);

		Texture2D t2d_normal = g_2DTextures[normalTextureIndex];
		float3 normalTS = normalize(t2d_normal.Sample(g_Sampler, input.texCoord).rgb * 2.0f - 1.0f);
		normalWS = normalize(mul(normalTS, TBN));
	}

	float transparency = 1.0;
	float3 out_albedo = materialCBuffer.albedo.rgb;
	int albedoTextureIndex = materialCBuffer.m_TextureIndices_1;
	if (albedoTextureIndex != -1)
	{
		Texture2D t2d_albedo = g_2DTextures[albedoTextureIndex];
		out_albedo = t2d_albedo.Sample(g_Sampler, input.texCoord).rgb;
		// @TODO: weird discard result
		// transparency = l_albedo.a;
		// if (transparency < 0.1)
		// {
		// 	discard;
		// }
		// else
		// {
		// 	out_albedo = l_albedo.rgb;
		// }
	}

	float out_metallic = materialCBuffer.MRAT.r;
	int metallicTextureIndex = materialCBuffer.m_TextureIndices_2;
	if (metallicTextureIndex != -1)
	{
		Texture2D t2d_metallic = g_2DTextures[albedoTextureIndex];
		out_metallic = t2d_metallic.Sample(g_Sampler, input.texCoord).r;
	}

	float out_roughness = materialCBuffer.MRAT.g;
	int roughnessTextureIndex = materialCBuffer.m_TextureIndices_3;
	if (roughnessTextureIndex != -1)
	{
		Texture2D t2d_roughness = g_2DTextures[roughnessTextureIndex];
		out_roughness = t2d_roughness.Sample(g_Sampler, input.texCoord).r;
	}

	float out_AO = materialCBuffer.MRAT.b;
	int aoTextureIndex = materialCBuffer.m_TextureIndices_4;
	if (aoTextureIndex != -1)
	{
		Texture2D t2d_ao = g_2DTextures[aoTextureIndex];
		out_AO = t2d_ao.Sample(g_Sampler, input.texCoord).r;
	}

	float4 motionVec = (input.posCS_orig / input.posCS_orig.w - input.posCS_prev / input.posCS_prev.w);

	output.opaquePassRT0 = float4(input.posWS, 1.0);
	output.opaquePassRT1 = float4(normalWS, out_metallic);
	output.opaquePassRT2 = float4(out_albedo, out_roughness);
	output.opaquePassRT3 = float4(motionVec.xy * 0.5, out_AO, transparency);

	return output;
}