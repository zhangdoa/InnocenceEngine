// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 0)]]
cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(1, 1)]]
StructuredBuffer<GPUModelData> g_ModelDataBuffer : register(t1);

[[vk::binding(2, 1)]]
StructuredBuffer<Material_CB> g_Materials : register(t2);

[[vk::binding(3, 1)]]
Texture2D g_2DTextures[] : register(t3);

[[vk::binding(0, 2)]]
SamplerState g_Sampler : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float2 texCoord : TEXCOORD;
	uint rtvId : SV_RenderTargetArrayIndex;
};

struct PixelOutputType
{
	float4 sunShadowPass : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	float depth = input.posCS.z;

	uint materialIndex = g_ModelDataBuffer[m_ObjectIndex].m_MaterialIndex;
	Material_CB materialCBuffer = g_Materials[materialIndex];

	float transparency;
	uint albedoTextureIndex = materialCBuffer.m_TextureIndices_1;
	if (albedoTextureIndex != INVALID_TEXTURE_INDEX)
	{
		Texture2D t2d_albedo = g_2DTextures[albedoTextureIndex];
		float4 l_albedo = t2d_albedo.Sample(g_Sampler, input.texCoord);
		transparency = l_albedo.a;
	}
	else
	{
		transparency = materialCBuffer.albedo.a;
	}

	if (transparency == 0.0)
	{
		discard;
	}

	output.sunShadowPass = float4(depth, depth * depth, 0.0, 1.0);

	return output;
}