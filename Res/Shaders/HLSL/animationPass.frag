// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 1)]]
Texture2D t2d_normal : register(t0);
[[vk::binding(1, 1)]]
Texture2D t2d_albedo : register(t1);
[[vk::binding(2, 1)]]
Texture2D t2d_metallic : register(t2);
[[vk::binding(3, 1)]]
Texture2D t2d_roughness : register(t3);
[[vk::binding(4, 1)]]
Texture2D t2d_ao : register(t4);

[[vk::binding(0, 2)]]
SamplerState SampleTypeWrap : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION_ORIG;
	float4 posCS_prev : POSITION_PREV;
	float3 posWS : POSITION;
	float2 texCoord : TEXCOORD;
	float3 normal : NORMAL;
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

	float3 normalInWorldSpace;
	if (materialCBuffer.textureSlotMask & 0x00000001)
	{
		// get edge vectors of the pixel triangle
		float3 dp1 = ddx_fine(input.posWS);
		float3 dp2 = ddy_fine(input.posWS);
		float2 duv1 = ddx_fine(input.texCoord);
		float2 duv2 = ddy_fine(input.texCoord);

		// solve the linear system
		float3 N = normalize(input.normal);

		float3 dp2perp = cross(dp2, N);
		float3 dp1perp = cross(N, dp1);
		float3 T = -normalize(dp2perp * duv1.x + dp1perp * duv2.x);
		float3 B = -normalize(dp2perp * duv1.y + dp1perp * duv2.y);

		float3x3 TBN = float3x3(T, B, N);

		float3 normalInTangentSpace = normalize(t2d_normal.Sample(SampleTypeWrap, input.texCoord).rgb * 2.0f - 1.0f);
		normalInWorldSpace = normalize(mul(normalInTangentSpace, TBN));
	}
	else
	{
		normalInWorldSpace = normalize(input.normal);
	}

	float transparency = 1.0;
	float3 out_Albedo;
	if (materialCBuffer.textureSlotMask & 0x00000002)
	{
		float4 l_albedo = t2d_albedo.Sample(SampleTypeWrap, input.texCoord);
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
		out_Metallic = t2d_metallic.Sample(SampleTypeWrap, input.texCoord).r;
	}
	else
	{
		out_Metallic = materialCBuffer.MRAT.r;
	}

	float out_Roughness;
	if (materialCBuffer.textureSlotMask & 0x00000008)
	{
		out_Roughness = t2d_roughness.Sample(SampleTypeWrap, input.texCoord).r;
	}
	else
	{
		out_Roughness = materialCBuffer.MRAT.g;
	}

	float out_AO;
	if (materialCBuffer.textureSlotMask & 0x00000010)
	{
		out_AO = t2d_ao.Sample(SampleTypeWrap, input.texCoord).r;
	}
	else
	{
		out_AO = materialCBuffer.MRAT.b;
	}

	output.opaquePassRT0 = float4(input.posWS, out_Metallic);
	output.opaquePassRT1 = float4(normalInWorldSpace, out_Roughness);
	output.opaquePassRT2 = float4(out_Albedo, out_AO);

	float4 motionVec = (input.posCS_orig / input.posCS_orig.w - input.posCS_prev / input.posCS_prev.w);

	output.opaquePassRT3 = float4(motionVec.xyz * 0.5, transparency);

  return output;
}