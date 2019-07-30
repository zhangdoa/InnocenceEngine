// shadertype=hlsl
#include "common.hlsl"

Texture2D in_opaquePassRT0 : register(t0);
Texture2D in_opaquePassRT1 : register(t1);
Texture2D in_opaquePassRT2 : register(t2);
Texture2D in_opaquePassRT3 : register(t3);

SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 lightPassRT0 : SV_Target0;
};

// Frostbite Engine model [https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
// ----------------------------------------------------------------------------
// punctual light attenuation
// ----------------------------------------------------------------------------
float smoothDistanceAtt(float squaredDistance, float invSqrAttRadius)
{
	float factor = squaredDistance * invSqrAttRadius;
	float smoothFactor = saturate(1.0 - factor * factor);
	return smoothFactor * smoothFactor;
}
float getDistanceAtt(float3 unormalizedLightVector, float invSqrAttRadius)
{
	float sqrDist = dot(unormalizedLightVector, unormalizedLightVector);
	float attenuation = 1.0 / (max(sqrDist, eps));
	attenuation *= smoothDistanceAtt(sqrDist, invSqrAttRadius);

	return attenuation;
}
// Specular Fresnel Component
// ----------------------------------------------------------------------------
float3 fresnelSchlick(float3 f0, float f90, float u)
{
	return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}
// Diffuse BRDF
// ----------------------------------------------------------------------------
float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	float energyBias = lerp(0, 0.5, linearRoughness);
	float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
	float3 f0 = float3(1.0, 1.0, 1.0);
	float lightScatter = fresnelSchlick(f0, fd90, NdotL).r;
	float viewScatter = fresnelSchlick(f0, fd90, NdotV).r;
	return lightScatter * viewScatter * energyFactor;
}
// Specular Geometry Component
// ----------------------------------------------------------------------------
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
	float alphaG2 = alphaG * alphaG;
	float Lambda_GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaG2) + alphaG2);
	float Lambda_GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaG2) + alphaG2);
	return 0.5 / max((Lambda_GGXV + Lambda_GGXL), 0.00001);
}
// Specular Distribution Component
// ----------------------------------------------------------------------------
float D_GGX(float NdotH, float roughness)
{
	// remapping to Quadratic curve
	float a = roughness * roughness;
	float a2 = a * a;
	float f = (NdotH * a2 - NdotH) * NdotH + 1;
	return a2 / pow(f, 2.0);
}
// BRDF
// ----------------------------------------------------------------------------
float3 CalcBRDF(float3 albedo, float metallic, float roughness, float3 F0, float NdotV, float LdotH, float NdotH, float NdotL)
{
	// Specular BRDF
	float F90 = 1.0;
	float3 F = fresnelSchlick(F0, F90, LdotH);
	float G = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 Fr = F * G * D;

	// Diffuse BRDF
	float Fd = DisneyDiffuse(NdotV, NdotL, LdotH, roughness * roughness);

	float3 BRDF = (Fd * albedo + Fr) / PI;

	return BRDF;
}
// ----------------------------------------------------------------------------
float3 getIlluminance(float3 albedo, float metallic, float roughness, float3 F0, float NdotV, float LdotH, float NdotH, float NdotL, float3 lightLuminance)
{
	float3 BRDF = CalcBRDF(albedo, metallic, roughness, F0, NdotV, LdotH, NdotH, NdotL);

	return BRDF * lightLuminance * NdotL;
}

// Spherical-Gaussians
// ----------------------------------------------------------------------------
struct SG
{
	float3 amplitude;
	float3 axis;
	float sharpness;
};

float3 EvaluateSG(in SG sg, in float3 dir)
{
	float cosAngle = dot(dir, sg.axis);
	return sg.amplitude * exp(sg.sharpness * (cosAngle - 1.0f));
}

SG SGProduct(in SG x, in SG y)
{
	float3 um = (x.sharpness * x.axis + y.sharpness * y.axis) /
		(x.sharpness + y.sharpness);
	float umLength = length(um);
	float lm = x.sharpness + y.sharpness;

	SG res;
	res.axis = um * (1.0f / umLength);
	res.sharpness = lm * umLength;
	res.amplitude = x.amplitude * y.amplitude *
		exp(lm * (umLength - 1.0f));

	return res;
}

float3 SGIntegral(in SG sg)
{
	float expTerm = 1.0f - exp(-2.0f * sg.sharpness);
	return 2 * PI * (sg.amplitude / sg.sharpness) * expTerm;
}

float3 SGInnerProduct(in SG x, in SG y)
{
	float umLength = length(x.sharpness * x.axis +
		y.sharpness * y.axis);
	float3 expo = exp(umLength - x.sharpness - y.sharpness) *
		x.amplitude * y.amplitude;
	float other = 1.0f - exp(-2.0f * umLength);
	return (2.0f * PI * expo * other) / umLength;
}

float SGsharpnessFromThreshold(in float amplitude,
	in float epsilon,
	in float cosTheta)
{
	return (log(epsilon) - log(amplitude)) / (cosTheta - 1.0f);
}

float3 SGIrradianceFitted(in SG lightingLobe, in float3 normal)
{
	const float muDotN = dot(lightingLobe.axis, normal);
	const float lambda = lightingLobe.sharpness;

	const float c0 = 0.36f;
	const float c1 = 1.0f / (4.0f * c0);

	float eml = exp(-lambda);
	float em2l = eml * eml;
	float rl = rcp(lambda);

	float scale = 1.0f + 2.0f * em2l - rl;
	float bias = (eml - em2l) * rl - em2l;

	float x = sqrt(1.0f - scale);
	float x0 = c0 * muDotN;
	float x1 = c1 * x;

	float n = x0 + x1;

	float y = saturate(muDotN);
	if (abs(x0) <= x1)
		y = n * n / x;

	float result = scale * y + bias;

	return result * SGIntegral(lightingLobe);
}

float3 SGGetIlluminance(SG lightingLobe, float3 albedo, float metallic, float roughness, float3 F0, float3 N, float3 V, float3 L)
{
	float3 H = normalize(V + L);

	float NdotV = max(dot(N, V), 0.0);
	float LdotH = max(dot(L, H), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float3 brdf = CalcBRDF(albedo, metallic, roughness, F0, NdotV, LdotH, NdotH, NdotL);

	return SGIrradianceFitted(lightingLobe, N) * brdf * NdotL;
}

SG SphereLightToSG(in float3 lightDir, in float radius, in float3 intensity, in float dist)
{
	SG sg;

	float r2 = radius * radius;
	float d2 = dist * dist;

	float lne = -2.230258509299f; // ln(0.1)
	sg.axis = normalize(lightDir);
	sg.sharpness = (-lne * d2) / r2;
	sg.amplitude = intensity;

	return sg;
}

SG DirectionalLightToSG(in float3 lightDir, in float3 intensity)
{
	SG sg;

	float lne = -2.230258509299f; // ln(0.1)
	sg.axis = normalize(lightDir);
	sg.sharpness = -lne;
	sg.amplitude = intensity;

	return sg;
}

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float4 GPassRT0 = in_opaquePassRT0.Sample(SampleTypePoint, input.texcoord);
	float4 GPassRT1 = in_opaquePassRT1.Sample(SampleTypePoint, input.texcoord);
	float4 GPassRT2 = in_opaquePassRT2.Sample(SampleTypePoint, input.texcoord);

	float3 posWS = GPassRT0.xyz;
	float metallic = GPassRT0.w;
	float3 normalWS = GPassRT1.xyz;
	float3 roughness = GPassRT1.w;
	float3 albedo = GPassRT2.xyz;
	float3 ao = GPassRT2.w;

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

	Lo += getIlluminance(albedo, metallic, roughness, F0, NdotV, LdotH, NdotH, NdotL, dirLight_luminance.xyz);

	SG SG_directionalLight = DirectionalLightToSG(normalize(-dirLight_dir.xyz), dirLight_luminance.xyz);
	//Lo += SGGetIlluminance(SG_directionalLight, albedo, metallic, roughness, F0, N, V, L);

	// point punctual light
	// Get the index of the current pixel in the light grid.
	//uint2 tileIndex = uint2(floor(input.position.xy / BLOCK_SIZE));

	// Get the start position and offset of the light in the light index list.
	//uint startOffset = in_LightGrid[tileIndex].x;
	//uint lightCount = in_LightGrid[tileIndex].y;

	//for (int i = 0; i < NR_POINT_LIGHTS; ++i)
	//{
	//	//uint lightIndex = in_LightIndexList[startOffset + i];
	//	pointLight light = pointLights[i];

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
	//	Lo += getIlluminance(albedo, metallic, roughness, F0, NdotV, LdotH, NdotH, NdotL, lightLuminance);

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

	//	Lo += getIlluminance(albedo, metallic, roughness, F0, NdotV, LdotH, NdotH, NdotL, illuminance * sphereLights[i].luminance.xyz);
	//}

	output.lightPassRT0 = float4(Lo, 1.0);
	return output;
}