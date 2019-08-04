// shadertype=hlsl
#include "common/common.hlsl"

Texture2D in_opaquePassRT0 : register(t0);
Texture2D in_opaquePassRT1 : register(t1);
Texture2D in_opaquePassRT2 : register(t2);
Texture2D in_opaquePassRT3 : register(t3);
Texture2D in_BRDFLUT : register(t4);
Texture2D in_BRDFMSLUT : register(t5);
Texture2D in_SSAO : register(t6);
Texture2DArray in_SunShadow : register(t7);
StructuredBuffer<uint> in_LightIndexList : register(t8);
Texture2D<uint2> in_LightGrid : register(t9);

SamplerState SampleTypePoint : register(s0);

#include "common/BRDF.hlsl"
#include "common/shadowResolver.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 lightPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float4 GPassRT0 = in_opaquePassRT0.Sample(SampleTypePoint, input.texcoord);
	float4 GPassRT1 = in_opaquePassRT1.Sample(SampleTypePoint, input.texcoord);
	float4 GPassRT2 = in_opaquePassRT2.Sample(SampleTypePoint, input.texcoord);
	float SSAO = in_SSAO.Sample(SampleTypePoint, input.texcoord).x;

	float3 posWS = GPassRT0.xyz;
	float metallic = GPassRT0.w;
	float3 normalWS = GPassRT1.xyz;
	float roughness = GPassRT1.w;
	float3 albedo = GPassRT2.xyz;
	float ao = GPassRT2.w;
	ao *= pow(SSAO, 2.0f);

	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo, metallic);

	float3 N = normalize(normalWS);
	float3 V = normalize(cam_globalPos.xyz - posWS);
	float NdotV = max(dot(N, V), 0.0);

	float3 Lo = float3(0, 0, 0);

	// direction light, sun light
	float3 L = normalize(-dirLight_dir.xyz);
	float3 H = normalize(V + L);

	float LdotH = max(dot(L, H), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, roughness, metallic, F0, albedo, dirLight_luminance.xyz);
	//Lo *= 1.0 - SunShadowResolver(posWS);

	//SG SG_directionalLight = DirectionalLightToSG(normalize(-dirLight_dir.xyz), dirLight_luminance.xyz);
	//Lo += SGGetIlluminance(SG_directionalLight, albedo, metallic, roughness, F0, N, V, L);

	//// point punctual light
	//// Get the index of the current pixel in the light grid.
	//uint2 tileIndex = uint2(floor(input.position.xy / BLOCK_SIZE));

	//// Get the start position and offset of the light in the light index list.
	//uint startOffset = in_LightGrid[tileIndex].x;
	//uint lightCount = in_LightGrid[tileIndex].y;

	//for (int i = 0; i < lightCount; ++i)
	//{
	//	uint lightIndex = in_LightIndexList[startOffset + i];
	//	pointLight light = pointLights[lightIndex];

	//	float3 unormalizedL = light.position.xyz - posWS;
	//	float lightAttRadius = light.luminance.w;

	//	L = normalize(unormalizedL);
	//	H = normalize(V + L);

	//	LdotH = max(dot(L, H), 0.0);
	//	NdotH = max(dot(N, H), 0.0);
	//	NdotL = max(dot(N, L), 0.0);

	//	float attenuation = 1.0;
	//	float invSqrAttRadius = 1.0 / max(lightAttRadius * lightAttRadius, eps);
	//	attenuation *= getDistanceAtt(unormalizedL, invSqrAttRadius);

	//	float3 lightLuminance = light.luminance.xyz * attenuation;
	//	Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, roughness, F0, albedo, lightLuminance);

	//	//use 1cm sphere light to represent point light
	//	//SG SG_pointLight = SphereLightToSG(L, 0.01, lightLuminance, distance);
	//	//Lo += SGGetIlluminance(SG_pointLight, albedo, metallic, roughness, F0, N, V, L);
	//}

	//// sphere area light
	//for (int i = 0; i < NR_SPHERE_LIGHTS; ++i)
	//{
	//	float3 unormalizedL = sphereLights[i].position.xyz - posWS;
	//	float lightSphereRadius = sphereLights[i].luminance.w;

	//	L = normalize(unormalizedL);
	//	H = normalize(V + L);

	//	LdotH = max(dot(L, H), 0.0);
	//	NdotH = max(dot(N, H), 0.0);
	//	NdotL = max(dot(N, L), 0.0);

	//	float sqrDist = dot(unormalizedL, unormalizedL);

	//	float Beta = acos(NdotL);
	//	float H2 = sqrt(sqrDist);
	//	float h = H2 / lightSphereRadius;
	//	float x = sqrt(max(h * h - 1, eps));
	//	float y = -x * (1 / tan(Beta));
	//	//y = clamp(y, -1.0, 1.0);
	//	float illuminance = 0;

	//	if (h * cos(Beta) > 1)
	//	{
	//		illuminance = cos(Beta) / (h * h);
	//	}
	//	else
	//	{
	//		illuminance = (1 / max(PI * h * h, eps))
	//			* (cos(Beta) * acos(y) - x * sin(Beta) * sqrt(max(1 - y * y, eps)))
	//			+ (1 / PI) * atan((sin(Beta) * sqrt(max(1 - y * y, eps)) / x));
	//	}
	//	illuminance *= PI;

	//	Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, roughness, F0, albedo, illuminance * sphereLights[i].luminance.xyz);
	//}

	// ambient occlusion
	Lo *= ao;

	output.lightPassRT0 = float4(Lo, 1.0);
	return output;
}