// shadertype=hlsl

cbuffer materialCBuffer : register(b2)
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
	normalInWorldSpace = normalize(input.frag_Normal);

	float transparency = 1.0;
	float3 out_Albedo;
	out_Albedo = albedo.rgb;

	float out_Metallic;
	out_Metallic = MRAT.r;

	float out_Roughness;
	out_Roughness = MRAT.g;

	float out_AO;
	out_AO = MRAT.b;

	output.geometryPassRT0 = float4(input.frag_WorldSpacePos, out_Metallic);
	output.geometryPassRT1 = float4(normalInWorldSpace, out_Roughness);
	output.geometryPassRT2 = float4(out_Albedo, out_AO);

	float4 motionVec = (input.frag_ClipSpacePos_orig / input.frag_ClipSpacePos_orig.w - input.frag_ClipSpacePos_prev / input.frag_ClipSpacePos_prev.w);

	output.geometryPassRT3 = float4(motionVec.xyz, transparency);

  return output;
}