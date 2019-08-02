// Oren-Nayar diffuse BRDF [https://github.com/glslify/glsl-diffuse-oren-nayar]
// ----------------------------------------------------------------------------
float orenNayarDiffuse(float LdotV, float NdotL, float NdotV, float roughness)
{
	float s = LdotV - NdotL * NdotV;
	float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));

	float sigma2 = roughness * roughness;
	float A = 1.0 - (0.5 * sigma2 / (sigma2 + 0.33));
	float B = 0.45 * sigma2 / (sigma2 + 0.09);

	return max(0.0, NdotL) * (A + B * s / t);
}
// Frostbite Engine model [https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
// ----------------------------------------------------------------------------
// punctual light attenuation
// ----------------------------------------------------------------------------
float smoothDistanceAtt(float squaredDistance, float invSqrAttRadius)
{
	float factor = squaredDistance * invSqrAttRadius;
	float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);
	return smoothFactor * smoothFactor;
}
float getDistanceAtt(vec3 unormalizedLightVector, float invSqrAttRadius)
{
	float sqrDist = dot(unormalizedLightVector, unormalizedLightVector);
	float attenuation = 1.0 / (max(sqrDist, eps));
	attenuation *= smoothDistanceAtt(sqrDist, invSqrAttRadius);

	return attenuation;
}
// Specular Fresnel Component
// ----------------------------------------------------------------------------
vec3 fr_F_Schlick(vec3 f0, float f90, float u)
{
	return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}
// Diffuse BRDF
// ----------------------------------------------------------------------------
float fd_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	float NdotV_ = max(NdotV, eps);
	float NdotL_ = max(NdotL, eps);
	float energyBias = mix(0.0, 0.5, linearRoughness);
	float energyFactor = mix(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
	vec3 f0 = vec3(1.0, 1.0, 1.0);
	float lightScatter = fr_F_Schlick(f0, fd90, NdotL_).r;
	float viewScatter = fr_F_Schlick(f0, fd90, NdotV_).r;
	return lightScatter * viewScatter * energyFactor / PI;
}
// Specular Visibility Component
// ----------------------------------------------------------------------------
float fr_V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
	float alphaG2 = alphaG * alphaG;
	float NdotV_ = max(NdotV, eps);
	float	NdotL_ = max(NdotL, eps);
	float Lambda_GGXV = NdotL_ * sqrt(NdotV_ * NdotV_ * (1.0 - alphaG2) + alphaG2);
	float Lambda_GGXL = NdotV_ * sqrt(NdotL_ * NdotL_ * (1.0 - alphaG2) + alphaG2);
	return 0.5 / (Lambda_GGXV + Lambda_GGXL);
}
// Specular Distribution Component
// ----------------------------------------------------------------------------
float fr_D_GGX(float NdotH, float roughness)
{
	// remapping to Quadratic curve
	float a = roughness * roughness;
	float a2 = a * a;
	float f = (NdotH * a2 - NdotH) * NdotH + 1;
	return a2 / (pow(f, 2.0));
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
	float k = (r*r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
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
vec3 AverangeFresnel(vec3 F0)
{
	return 20.0 * F0 / 21.0 + 1.0 / 21.0;
}
vec3 getFrMS(sampler2D BRDFLUT, sampler2D BRDFMSLUT, float NdotL, float NdotV, vec3 F0, float roughness)
{
	float alpha = roughness * roughness;
	vec3 f_averange = AverangeFresnel(F0);
	float rsF1_averange = texture(BRDFMSLUT, vec2(0.0, alpha)).r;
	float rsF1_l = texture(BRDFLUT, vec2(NdotL, alpha)).b;
	float rsF1_v = texture(BRDFLUT, vec2(NdotV, alpha)).b;

	vec3 frMS = vec3(0.0);
	float beta1 = 1.0 - rsF1_averange;
	float beta2 = 1.0 - rsF1_l;
	float beta3 = 1.0 - rsF1_v;

	frMS = f_averange * rsF1_averange / (PI * beta1 * (vec3(1.0) - f_averange * beta1) + eps);
	frMS = frMS * beta2 * beta3;
	return frMS;
}
// IBL
// corrected Fresnel
// ----------------------------------------------------------------------------
vec3 fr_F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------
vec3 getBRDF(float NdotV, float LdotH, float NdotH, float NdotL, float roughness, vec3 F0, vec3 albedo)
{
	// Specular BRDF
	float f90 = 1.0;
	vec3 F = fr_F_Schlick(F0, f90, NdotV);
	float G = fr_V_SmithGGXCorrelated(NdotV, NdotL, roughness);
	float D = fr_D_GGX(NdotH, roughness);
	vec3 Frss = F * D * G / PI;

	// Real-Time Rendering", 4th edition, pg. 341, "9.8 BRDF Models for Surface Reflection, the 4 * NdV * NdL has already been cancelled by G function
	vec3 Frms = getFrMS(uni_brdfLUT, uni_brdfMSLUT, NdotL, NdotV, F0, roughness);

	vec3 Fr = Frss + Frms;

	// Diffuse BRDF
	vec3 Fd = fd_DisneyDiffuse(NdotV, NdotL, LdotH, roughness * roughness) * albedo;

	return (Fd + Fr);
}
// ----------------------------------------------------------------------------
vec3 getIlluminance(float NdotV, float LdotH, float NdotH, float NdotL, float roughness, vec3 F0, vec3 albedo, vec3 lightLuminance)
{
	vec3 BRDF = getBRDF(NdotV, LdotH, NdotH, NdotL, roughness, F0, albedo);

	return BRDF * lightLuminance * NdotL;
}