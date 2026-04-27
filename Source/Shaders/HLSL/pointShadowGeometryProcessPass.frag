// shadertype=hlsl
//
// Cube-shadow caster — PS pass.
// Outputs packed linear distance `(linearDist, linearDist², 0, 1)` to RG32F.
// Linear distance (vs. perspective z) is the canonical omnidirectional shadow
// metric: uniform across all 6 cube faces, no metric drift, simple to compare
// in the resolver. Mirrors sun shadow's packed-depth idiom (sunShadowGeometryProcessPass.frag:61)
// but with linear depth instead of clip-space z.
#include "common/common.hlsl"

[[vk::binding(0, 0)]]
cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(1, 0)]]
cbuffer PointShadowCBuffer : register(b1)
{
	PointShadow_CB g_PointShadows[NR_POINT_SHADOWS];
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
	float3 posWS : POSITION0;
	float2 texCoord : TEXCOORD;
	uint lightSlot : TEXCOORD1;
	uint rtvId : SV_RenderTargetArrayIndex;
};

struct PixelOutputType
{
	float2 packedDepth : SV_Target0;
};

PixelOutputType main(PixelInputType input)
{
	PixelOutputType output;

	uint materialIndex = g_ModelDataBuffer[m_ObjectIndex].m_MaterialIndex;
	Material_CB materialCBuffer = g_Materials[materialIndex];

	// Alpha-test: same idiom as sunShadowGeometryProcessPass.frag:42-58 — thin
	// or transparent meshes (curtains, foliage, decals) discard so the shadow
	// map records light leaks correctly through cut-out geometry.
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

	// Linear distance from light to fragment, normalized by attenuation range
	// so the resolver can use a unit-domain comparison without per-light
	// rescaling. The matching consumer is shadowResolver.hlsl::PointShadowResolver.
	float3 lightPos = g_PointShadows[input.lightSlot].lightPosWS_range.xyz;
	float range = g_PointShadows[input.lightSlot].lightPosWS_range.w;
	float linearDist = length(input.posWS - lightPos) / max(range, EPSILON);

	output.packedDepth = float2(linearDist, linearDist * linearDist);
	return output;
}
