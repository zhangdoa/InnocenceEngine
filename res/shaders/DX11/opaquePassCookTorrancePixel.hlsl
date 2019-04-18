// shadertype=hlsl

Texture2D t2d_normal : register(t0);
Texture2D t2d_albedo : register(t1);
Texture2D t2d_metallic : register(t2);
Texture2D t2d_roughness : register(t3);
Texture2D t2d_ao : register(t4);

SamplerState SampleTypeWrap : register(s0);

cbuffer materialCBuffer : register(b0)
{
	float4 albedo;
	float4 MRAT;
	bool useNormalTexture;
	bool useAlbedoTexture;
	bool useMetallicTexture;
	bool useRoughnessTexture;
	bool useAOTexture;
	bool padding1;
	bool padding2;
	bool padding3;
};

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
	float4 geometryPassRT0 : SV_Target0;
	float4 geometryPassRT1 : SV_Target1;
	float4 geometryPassRT2 : SV_Target2;
	float4 geometryPassRT3 : SV_Target3;
};

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float3 normalInWorldSpace;
	if (useNormalTexture)
	{
		// get edge vectors of the pixel triangle
		float3 dp1 = ddx_fine(input.frag_WorldSpacePos);
		float3 dp2 = ddy_fine(input.frag_WorldSpacePos);
		float2 duv1 = ddx_fine(input.frag_TexCoord);
		float2 duv2 = ddy_fine(input.frag_TexCoord);

		// solve the linear system
		float3 N = normalize(input.frag_Normal);

		float3 dp2perp = cross(dp2, N);
		float3 dp1perp = cross(N, dp1);
		float3 T = -normalize(dp2perp * duv1.x + dp1perp * duv2.x);
		float3 B = -normalize(dp2perp * duv1.y + dp1perp * duv2.y);

		float3x3 TBN = float3x3(T, B, N);

		float3 normalInTangentSpace = normalize(t2d_normal.Sample(SampleTypeWrap, input.frag_TexCoord).rgb * 2.0f - 1.0f);
		normalInWorldSpace = normalize(mul(normalInTangentSpace, TBN));
	}
	else
	{
		normalInWorldSpace = normalize(input.frag_Normal);
	}

	float transparency = 1.0;
	float3 out_Albedo;
	if (useAlbedoTexture)
	{
		float4 l_albedo = t2d_albedo.Sample(SampleTypeWrap, input.frag_TexCoord);
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
		out_Metallic = t2d_metallic.Sample(SampleTypeWrap, input.frag_TexCoord).r;
	}
	else
	{
		out_Metallic = MRAT.r;
	}

	float out_Roughness;
	if (useRoughnessTexture)
	{
		out_Roughness = t2d_roughness.Sample(SampleTypeWrap, input.frag_TexCoord).r;
	}
	else
	{
		out_Roughness = MRAT.g;
	}

	float out_AO;
	if (useAOTexture)
	{
		out_AO = t2d_ao.Sample(SampleTypeWrap, input.frag_TexCoord).r;
	}
	else
	{
		out_AO = MRAT.b;
	}

	output.geometryPassRT0 = float4(input.frag_WorldSpacePos, out_Metallic);

	output.geometryPassRT1 = float4(normalInWorldSpace, out_Roughness);

	output.geometryPassRT2 = float4(out_Albedo, out_AO);

	float4 motionVec = (input.frag_ClipSpacePos_orig / input.frag_ClipSpacePos_orig.w - input.frag_ClipSpacePos_prev / input.frag_ClipSpacePos_prev.w);

	output.geometryPassRT3 = float4(motionVec.xyz, transparency);

  return output;
}