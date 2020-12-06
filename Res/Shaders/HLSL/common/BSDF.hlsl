// Frostbite Engine model [https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
// ----------------------------------------------------------------------------
// Punctual light attenuation
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
// Specular Geometry Component
// ----------------------------------------------------------------------------
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
	float alphaG2 = alphaG * alphaG;
	float Lambda_GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaG2) + alphaG2);
	float Lambda_GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaG2) + alphaG2);
	return 0.5 / max((Lambda_GGXV + Lambda_GGXL), eps);
}
// Specular Distribution Component
// ----------------------------------------------------------------------------
float D_GGX(float NdotH, float roughness)
{
	// remapping to Quadratic curve
	float a = roughness * roughness;
	float a2 = a * a;
	float f = (NdotH * a2 - NdotH) * NdotH + 1;
	return a2 / max(PI * pow(f, 2.0), eps);
}
// Diffuse BTDF
// Disney model [https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf]
// ----------------------------------------------------------------------------
float3 DisneyDiffuse2012(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	float energyBias = lerp(0, 0.5, linearRoughness);
	float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
	float3 f0 = float3(1.0, 1.0, 1.0);
	float3 lightScatter = fresnelSchlick(f0, fd90, NdotL);
	float3 viewScatter = fresnelSchlick(f0, fd90, NdotV);
	return lightScatter * viewScatter * energyFactor;
}
// ----------------------------------------------------------------------------
float DisneyDiffuse2015(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	float Fl = pow(1.0 - NdotL, 5.0);
	float Fv = pow(1.0 - NdotV, 5.0);
	float Rr = 2.0 * LdotH * LdotH * linearRoughness;
	float FLambert = (1 - 0.5 * Fl) * (1 - 0.5 * Fv);
	float FRetroReflection = Rr * (Fl + Fv + Fl * Fv * (Rr - 1.0));
	return FLambert + FRetroReflection;
}
// Unreal Engine model [https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf]
// ----------------------------------------------------------------------------
// Specular Distribution Component
// ----------------------------------------------------------------------------
float Unreal_DistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	// remapping to Quadratic curve
	float a2 = a * a;
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = denom * denom;

	return nom / denom;
}
// Specular Geometry Component
// ----------------------------------------------------------------------------
float Unreal_GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / max(denom, eps);
}
// ----------------------------------------------------------------------------
float Unreal_GeometrySmith(float NdotV, float NdotL, float roughness)
{
	float ggx2 = Unreal_GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = Unreal_GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}
// "Real-Time Rendering", 4th edition, pg. 346, "9.8.2 Multiple-Bounce Surface Reflection"
// ----------------------------------------------------------------------------
float3 AverangeFresnel(float3 F0)
{
	return (F0 * 20.0 / 21.0) + 1.0 / 21.0;
}
float3 getFrMS(Texture2D BRDFLUT, Texture2D BRDFMSLUT, SamplerState samplerTypePoint, float NdotL, float NdotV, float3 F0, float roughness)
{
	float3 f_averange = AverangeFresnel(F0);

	float rsF1_averange = BRDFMSLUT.SampleLevel(samplerTypePoint, float2(0.0, roughness), 0).r;

	float rsF1_l = BRDFLUT.SampleLevel(samplerTypePoint, float2(NdotL, roughness), 0).b;
	float rsF1_v = BRDFLUT.SampleLevel(samplerTypePoint, float2(NdotV, roughness), 0).b;

	float beta1 = 1.0 - rsF1_averange;
	float beta2 = 1.0 - rsF1_l;
	float beta3 = 1.0 - rsF1_v;
	float frMS = beta2 * beta3 / (beta1 * PI);
	// [https://blog.selfshadow.com/2018/06/04/multi-faceted-part-2/]
	float3 fresnelMultiplier = f_averange * f_averange * rsF1_averange / max((float3(1.0, 1.0, 1.0) - f_averange * beta1), eps);

	return frMS * fresnelMultiplier;
}
// ----------------------------------------------------------------------------
float3 getBRDF(Texture2D BRDFLUT, Texture2D BRDFMSLUT, SamplerState samplerTypePoint, float NdotV, float NdotL, float NdotH, float LdotH, float roughness, float3 F0, float3 FresnelFactor)
{
	float G = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 Frss = FresnelFactor * G * D;

	// Real-Time Rendering", 4th edition, pg. 341, "9.8 BRDF Models for Surface Reflection, the 4 * NdV * NdL has already been cancelled by G function
	float3 Frms = getFrMS(BRDFLUT, BRDFMSLUT, samplerTypePoint, NdotL, NdotV, F0, roughness);

	float3 Fr = Frss + Frms;

	return Fr;
}
// ----------------------------------------------------------------------------
float3 getBRDF_Indirect(Texture2D BRDFLUT, Texture2D BRDFMSLUT, SamplerState samplerTypePoint, float NdotV, float NdotL, float NdotH, float LdotH, float roughness, float3 F0, float3 FresnelFactor)
{
	float G = Unreal_GeometrySmith(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 Frss = FresnelFactor * G * D;

	// Real-Time Rendering", 4th edition, pg. 341, "9.8 BRDF Models for Surface Reflection, the 4 * NdV * NdL has already been cancelled by G function
	float3 Frms = getFrMS(BRDFLUT, BRDFMSLUT, samplerTypePoint, NdotL, NdotV, F0, roughness);

	float3 Fr = Frss + Frms;

	return Fr;
}
float3 getBTDF(float NdotV, float NdotL, float LdotH, float roughness, float metallic, float3 FresnelFactor, float3 albedo)
{
	float3 kD = float3(1.0, 1.0, 1.0) - FresnelFactor;

	kD *= 1.0 - metallic;

	float3 Ft = DisneyDiffuse2015(NdotV, NdotL, LdotH, roughness * roughness) * albedo / PI;

	return kD * Ft;
}
float3 getBSDF(Texture2D BRDFLUT, Texture2D BRDFMSLUT, SamplerState samplerTypePoint, float NdotV, float NdotL, float NdotH, float LdotH, float roughness, float metallic, float3 F0, float3 FresnelFactor, float3 albedo)
{
	float3 Ft = getBTDF(NdotV, NdotL, LdotH, roughness, metallic, FresnelFactor, albedo);
	float3 Fr = getBRDF(BRDFLUT, BRDFMSLUT, samplerTypePoint, NdotV, NdotL, NdotH, LdotH, roughness, F0, FresnelFactor);
	return (Ft + Fr);
}
// ----------------------------------------------------------------------------
float3 getOutLuminance(Texture2D BRDFLUT, Texture2D BRDFMSLUT, SamplerState samplerTypePoint, float NdotV, float NdotL, float NdotH, float LdotH, float roughness, float metallic, float3 F0, float3 albedo, float3 luminousFlux)
{
	float F90 = 1.0;
	float3 FresnelFactor = fresnelSchlick(F0, F90, LdotH);

	float3 BRDF = getBSDF(BRDFLUT, BRDFMSLUT, samplerTypePoint, NdotV, NdotL, NdotH, LdotH, roughness, metallic, F0, FresnelFactor, albedo);

	return BRDF * luminousFlux * NdotL;
}