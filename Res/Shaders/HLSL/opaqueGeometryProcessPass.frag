// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 0)]]
cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(1, 0)]]
cbuffer PerFrameConstantBuffer : register(b1)
{
    PerFrame_CB g_Frame;
}

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
	uint normalTextureIndex = materialCBuffer.m_TextureIndices_0;
	if (normalTextureIndex != INVALID_TEXTURE_INDEX)
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
	float3 out_albedo = float3(0.0, 0.0, 0.0);
	uint albedoTextureIndex = materialCBuffer.m_TextureIndices_1;
	if (albedoTextureIndex != INVALID_TEXTURE_INDEX)
	{
		Texture2D t2d_albedo = g_2DTextures[albedoTextureIndex];
		out_albedo = t2d_albedo.Sample(g_Sampler, input.texCoord).rgb;
	}
	else
	{
		out_albedo = materialCBuffer.albedo.rgb;
	}

	float out_metallic = materialCBuffer.MRAT.r;
	uint metallicTextureIndex = materialCBuffer.m_TextureIndices_2;
	if (metallicTextureIndex != INVALID_TEXTURE_INDEX)
	{
		Texture2D t2d_metallic = g_2DTextures[metallicTextureIndex];
		out_metallic = t2d_metallic.Sample(g_Sampler, input.texCoord).r;
	}

	float out_roughness = materialCBuffer.MRAT.g;
	uint roughnessTextureIndex = materialCBuffer.m_TextureIndices_3;
	if (roughnessTextureIndex != INVALID_TEXTURE_INDEX)
	{
		Texture2D t2d_roughness = g_2DTextures[roughnessTextureIndex];
		out_roughness = t2d_roughness.Sample(g_Sampler, input.texCoord).r;
	}

	float out_AO = materialCBuffer.MRAT.b;
	uint aoTextureIndex = materialCBuffer.m_TextureIndices_4;
	if (aoTextureIndex != INVALID_TEXTURE_INDEX)
	{
		Texture2D t2d_ao = g_2DTextures[aoTextureIndex];
		out_AO = t2d_ao.Sample(g_Sampler, input.texCoord).r;
	}

	float4 posWS_prev = float4(input.posWS, 1.0);
	float4 posVS_prev = mul(posWS_prev, g_Frame.v_prev);
	float4 posCS_prev = mul(posVS_prev, g_Frame.p_original);

	float w_orig = max(abs(input.posCS_orig.w), EPSILON);
	float w_prev = max(abs(posCS_prev.w), EPSILON);

	float2 screenPos_orig = (input.posCS_orig.xy / w_orig) * 0.5 + 0.5;
	float2 screenPos_prev = (posCS_prev.xy / w_prev) * 0.5 + 0.5;

	screenPos_orig.y = 1.0 - screenPos_orig.y; // Flip Y for screen-space
	screenPos_prev.y = 1.0 - screenPos_prev.y;

	screenPos_orig *= g_Frame.viewportSize.xy;
	screenPos_prev *= g_Frame.viewportSize.xy;

	float2 motionVec = screenPos_prev - screenPos_orig;

	output.opaquePassRT0 = float4(input.posWS, 1.0);
	output.opaquePassRT1 = float4(normalWS, out_metallic);
	output.opaquePassRT2 = float4(out_albedo, out_roughness);
	output.opaquePassRT3 = float4(motionVec.xy, out_AO, transparency);

	return output;
}