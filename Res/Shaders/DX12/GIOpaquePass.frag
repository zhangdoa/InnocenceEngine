// shadertype=hlsl
#include "common/common.hlsl"

Texture2D t2d_normal : register(t0);
Texture2D t2d_albedo : register(t1);
Texture2D t2d_metallic : register(t2);
Texture2D t2d_roughness : register(t3);
Texture2D t2d_ao : register(t4);

SamplerState SampleTypeWrap : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION;
	float2 texcoord : TEXCOORD;
	float4 normal : NORMAL;
	uint rtvId : SV_RenderTargetArrayIndex;
};

struct PixelOutputType
{
	float4 opaquePassRT0 : SV_Target0;
	float4 opaquePassRT1 : SV_Target1;
	float4 opaquePassRT2 : SV_Target2;
};

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float3 normalInWorldSpace;
	if (useNormalTexture)
	{
		// get edge vectors of the pixel triangle
		float3 dp1 = ddx_fine(input.posWS.xyz);
		float3 dp2 = ddy_fine(input.posWS.xyz);
		float2 duv1 = ddx_fine(input.texcoord);
		float2 duv2 = ddy_fine(input.texcoord);

		// solve the linear system
		float3 N = normalize(input.normal.xyz);

		float3 dp2perp = cross(dp2, N);
		float3 dp1perp = cross(N, dp1);
		float3 T = -normalize(dp2perp * duv1.x + dp1perp * duv2.x);
		float3 B = -normalize(dp2perp * duv1.y + dp1perp * duv2.y);

		float3x3 TBN = float3x3(T, B, N);

		float3 normalInTangentSpace = normalize(t2d_normal.Sample(SampleTypeWrap, input.texcoord).rgb * 2.0f - 1.0f);
		normalInWorldSpace = normalize(mul(normalInTangentSpace, TBN));
	}
	else
	{
		normalInWorldSpace = normalize(input.normal.xyz);
	}

	float transparency = 1.0;
	float3 out_Albedo;
	if (useAlbedoTexture)
	{
		float4 l_albedo = t2d_albedo.Sample(SampleTypeWrap, input.texcoord);
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
		out_Albedo = albedo.rgb;
	}

	float out_Metallic;
	if (useMetallicTexture)
	{
		out_Metallic = t2d_metallic.Sample(SampleTypeWrap, input.texcoord).r;
	}
	else
	{
		out_Metallic = MRAT.r;
	}

	float out_Roughness;
	if (useRoughnessTexture)
	{
		out_Roughness = t2d_roughness.Sample(SampleTypeWrap, input.texcoord).r;
	}
	else
	{
		out_Roughness = MRAT.g;
	}

	float out_AO;
	if (useAOTexture)
	{
		out_AO = t2d_ao.Sample(SampleTypeWrap, input.texcoord).r;
	}
	else
	{
		out_AO = MRAT.b;
	}

	output.opaquePassRT0 = float4(input.posWS.xyz, out_Metallic);

	output.opaquePassRT1 = float4(normalInWorldSpace, out_Roughness);

	output.opaquePassRT2 = float4(out_Albedo, out_AO);

  return output;
}