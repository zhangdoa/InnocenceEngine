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
SamplerState in_samplerTypeWrap : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION_ORIG;
	float4 posCS_prev : POSITION_PREV;
	float3 posWS : POSITION;
	float2 texCoord : TEXCOORD;
	float3 normalWS : NORMAL;
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

	float3 normalWS = normalize(input.normalWS);
	if (materialCBuffer.textureSlotMask & 0x00000001)
	{
		// get edge vectors of the pixel triangle
		float3 dp1 = ddx_fine(input.posWS);
		float3 dp2 = ddy_fine(input.posWS);
		float2 duv1 = ddx_fine(input.texCoord);
		float2 duv2 = ddy_fine(input.texCoord);

		// solve the linear system
		float3 N = normalize(input.normalWS);

		float3 dp2perp = cross(dp2, N);
		float3 dp1perp = cross(N, dp1);
		float3 T = -normalize(dp2perp * duv1.x + dp1perp * duv2.x);
		float3 B = -normalize(dp2perp * duv1.y + dp1perp * duv2.y);

		float3x3 TBN = float3x3(T, B, N);

		float3 normalTS = normalize(t2d_normal.Sample(in_samplerTypeWrap, input.texCoord).rgb * 2.0f - 1.0f);
		normalWS = normalize(mul(normalTS, TBN));
	}

	float transparency = 1.0;
	float3 out_albedo = materialCBuffer.albedo.rgb;
	if (materialCBuffer.textureSlotMask & 0x00000002)
	{
		out_albedo = t2d_albedo.Sample(in_samplerTypeWrap, input.texCoord).rgb;
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

	float out_metallic = out_metallic = materialCBuffer.MRAT.r;
	if (materialCBuffer.textureSlotMask & 0x00000004)
	{
		out_metallic = t2d_metallic.Sample(in_samplerTypeWrap, input.texCoord).r;
	}

	float out_roughness = materialCBuffer.MRAT.g;
	if (materialCBuffer.textureSlotMask & 0x00000008)
	{
		out_roughness = t2d_roughness.Sample(in_samplerTypeWrap, input.texCoord).r;
	}

	float out_AO = materialCBuffer.MRAT.b;
	if (materialCBuffer.textureSlotMask & 0x00000010)
	{
		out_AO = t2d_ao.Sample(in_samplerTypeWrap, input.texCoord).r;
	}

	float4 motionVec = (input.posCS_orig / input.posCS_orig.w - input.posCS_prev / input.posCS_prev.w);

	output.opaquePassRT0 = float4(input.posWS, out_metallic);
	output.opaquePassRT1 = float4(normalWS, out_roughness);
	output.opaquePassRT2 = float4(out_albedo, out_AO);
	output.opaquePassRT3 = float4(motionVec.xyz * 0.5, transparency);

  	return output;
}