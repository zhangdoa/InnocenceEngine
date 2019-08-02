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