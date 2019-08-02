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
float3 DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	float energyBias = lerp(0, 0.5, linearRoughness);
	float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
	float3 f0 = float3(1.0, 1.0, 1.0);
	float3 lightScatter = fresnelSchlick(f0, fd90, NdotL);
	float3 viewScatter = fresnelSchlick(f0, fd90, NdotV);
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
// "Real-Time Rendering", 4th edition, pg. 346, "9.8.2 Multiple-Bounce Surface Reflection"
// ----------------------------------------------------------------------------
float3 AverangeFresnel(float3 F0)
{
	return 20.0 * F0 / 21.0 + 1.0 / 21.0;
}
float3 getFrMS(Texture2D BRDFLUT, Texture2D BRDFMSLUT, float NdotL, float NdotV, float3 F0, float roughness)
{
	float alpha = roughness * roughness;
	float3 f_averange = AverangeFresnel(F0);
	float rsF1_averange = BRDFMSLUT.Sample(SampleTypePoint, float2(0.0, alpha)).r;
	float rsF1_l = BRDFLUT.Sample(SampleTypePoint, float2(NdotL, alpha)).b;
	float rsF1_v = BRDFLUT.Sample(SampleTypePoint, float2(NdotV, alpha)).b;

	float3 frMS = float3(0.0, 0.0, 0.0);
	float beta1 = 1.0 - rsF1_averange;
	float beta2 = 1.0 - rsF1_l;
	float beta3 = 1.0 - rsF1_v;

	frMS = f_averange * rsF1_averange / (PI * beta1 * (float3(1.0, 1.0, 1.0) - f_averange * beta1) + eps);
	frMS = frMS * beta2 * beta3;
	return frMS;
}
// ----------------------------------------------------------------------------
float3 getBRDF(float NdotV, float LdotH, float NdotH, float NdotL, float roughness, float3 F0, float3 albedo)
{
	// Specular BRDF
	float F90 = 1.0;
	float3 F = fresnelSchlick(F0, F90, LdotH);
	float G = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 Frss = F * G * D / PI;

	// Real-Time Rendering", 4th edition, pg. 341, "9.8 BRDF Models for Surface Reflection, the 4 * NdV * NdL has already been cancelled by G function
	float3 Frms = getFrMS(in_BRDFLUT, in_BRDFMSLUT, NdotL, NdotV, F0, roughness);

	float3 Fr = Frss + Frms;

	// Diffuse BRDF
	float3 Fd = DisneyDiffuse(NdotV, NdotL, LdotH, roughness * roughness) * albedo;

	return (Fd + Fr);
}
// ----------------------------------------------------------------------------
float3 getIlluminance(float NdotV, float LdotH, float NdotH, float NdotL, float roughness, float3 F0, float3 albedo, float3 lightLuminance)
{
	float3 BRDF = getBRDF(NdotV, LdotH, NdotH, NdotL, roughness, F0, albedo);

	return BRDF * lightLuminance * NdotL;
}