// ============================================================================
// BRDF/BSDF Utility Functions
// Based on Frostbite/Unreal/Disney models and "Real-Time Rendering" references.
// ============================================================================

// ----------------------------------------------------------------------------
// Smooth Distance Attenuation
//
// Computes a smooth attenuation factor based on the squared distance between
// a point and a light, and the light's inverse squared attenuation radius.
// This function first computes a factor (squaredDistance * invSqrAttRadius),
// then applies a smooth (squared) falloff.
// ----------------------------------------------------------------------------
float SmoothDistanceAttenuation(float squaredDistance, float invSqrAttRadius)
{
	float factor = squaredDistance * invSqrAttRadius;
	float smoothFactor = saturate(1.0 - factor * factor);
	return smoothFactor * smoothFactor;
}

// ----------------------------------------------------------------------------
// Calculate Distance Attenuation for a Punctual Light
//
// Uses the inverse-square law modulated by a smooth falloff to compute the 
// light attenuation based on distance. The light vector is assumed to be unnormalized.
// ----------------------------------------------------------------------------
float CalculateDistanceAttenuation(float3 unnormalizedLightVec, float invSqrAttRadius)
{
	float sqrDistance = dot(unnormalizedLightVec, unnormalizedLightVec);
	// Inverse-square attenuation (clamped by EPSILON)
	float attenuation = 1.0 / max(sqrDistance, EPSILON);
	// Modulate with a smooth falloff function
	attenuation *= SmoothDistanceAttenuation(sqrDistance, invSqrAttRadius);
	return attenuation;
}

// ----------------------------------------------------------------------------
// Fresnel Term (Schlick's Approximation)
//
// Computes the Fresnel reflectance using Schlick's approximation.
// - f0: Reflectance at normal incidence (usually the base reflectivity).
// - f90: Reflectance at grazing angles (commonly 1.0 for dielectrics).
// - cosTheta: Cosine of the angle between the view (or half-) vector and the normal.
// ----------------------------------------------------------------------------
float3 Fresnel_Schlick(float3 f0, float f90, float cosTheta)
{
	return f0 + (f90 - f0) * pow(1.0 - cosTheta, 5.0);
}

// ----------------------------------------------------------------------------
// Geometry Attenuation (Smith GGX Correlated)
//
// Computes the combined shadowing/masking function using the correlated 
// Smith method for the GGX microfacet distribution.
// - NdotL: Cosine between surface normal and light direction.
// - NdotV: Cosine between surface normal and view direction.
// - alpha: Surface roughness (or a related roughness parameter).
// ----------------------------------------------------------------------------
float GeometrySmithGGXCorrelated(float NdotL, float NdotV, float alpha)
{
	float alpha2 = alpha * alpha;
	float lambdaV = NdotL * sqrt(NdotV * NdotV * (1.0 - alpha2) + alpha2);
	float lambdaL = NdotV * sqrt(NdotL * NdotL * (1.0 - alpha2) + alpha2);
	return 0.5 / max((lambdaV + lambdaL), EPSILON);
}

// ----------------------------------------------------------------------------
// GGX Normal Distribution Function (NDF)
//
// Computes the GGX microfacet distribution using a quadratic remapping of the 
// roughness parameter. 
// - NdotH: Cosine between surface normal and half-vector.
// - roughness: Surface roughness.
// ----------------------------------------------------------------------------
float D_GGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	// Remapping function: f = (NdotH * a2 - NdotH) * NdotH + 1
	float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
	return a2 / max(PI * pow(f, 2.0), EPSILON);
}

// ----------------------------------------------------------------------------
// Disney Diffuse Model (2012)
// 
// Computes a diffuse reflectance term that accounts for energy conservation 
// and retro-reflection based on the Disney 2012 diffuse model.
// - NdotV: Cosine between surface normal and view direction.
// - NdotL: Cosine between surface normal and light direction.
// - LdotH: Cosine between light direction and half-vector.
// - linearRoughness: Perceptual roughness (linear space).
// ----------------------------------------------------------------------------
float3 DisneyDiffuse2012(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	// Energy bias and factor are interpolated with roughness.
	float energyBias = lerp(0.0, 0.5, linearRoughness);
	float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
	// f0 is taken as a constant white (since it modulates the Lambert term)
	float3 f0 = float3(1.0, 1.0, 1.0);
	float3 lightScatter = Fresnel_Schlick(f0, fd90, NdotL);
	float3 viewScatter = Fresnel_Schlick(f0, fd90, NdotV);
	return lightScatter * viewScatter * energyFactor;
}

// ----------------------------------------------------------------------------
// Disney Diffuse Model (2015 / Burley)
// 
// Computes a diffuse term using the Burley/Disney 2015 model which includes 
// retro-reflection effects. This version uses power functions on (1 - NdotL)
// and (1 - NdotV) to create a smooth falloff.
// - NdotV: Cosine between surface normal and view direction.
// - NdotL: Cosine between surface normal and light direction.
// - LdotH: Cosine between light direction and half-vector.
// - linearRoughness: Perceptual roughness (in linear space).
// ----------------------------------------------------------------------------
float DisneyDiffuse2015(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
	float F_l = pow(1.0 - NdotL, 5.0);
	float F_v = pow(1.0 - NdotV, 5.0);
	float retroReflect = 2.0 * LdotH * LdotH * linearRoughness;
	float FLambert = (1.0 - 0.5 * F_l) * (1.0 - 0.5 * F_v);
	float FRetroReflection = retroReflect * (F_l + F_v + F_l * F_v * (retroReflect - 1.0));
	return FLambert + FRetroReflection;
}

// ----------------------------------------------------------------------------
// Unreal Engine GGX Normal Distribution Function
//
// An alternative GGX NDF used in Unreal Engine. It uses a similar quadratic 
// remapping but omits the π normalization factor present in other variants.
// - NdotH: Cosine between surface normal and half-vector.
// - roughness: Surface roughness.
// ----------------------------------------------------------------------------
float Unreal_GGXDistribution(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH2 = NdotH * NdotH;
	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = denom * denom;
	return nom / max(denom, EPSILON);
}

// ----------------------------------------------------------------------------
// Unreal Engine Geometry Term using Schlick-GGX
//
// Computes the geometry (shadowing/masking) term for a single direction based
// on Schlick's approximation, as used in Unreal Engine.
// - NdotX: Cosine between surface normal and the direction (view or light).
// - roughness: Surface roughness.
// ----------------------------------------------------------------------------
float Unreal_GeometrySchlickGGX(float NdotX, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return NdotX / max(NdotX * (1.0 - k) + k, EPSILON);
}

// ----------------------------------------------------------------------------
// Unreal Engine Combined Geometry Term (Smith Method)
//
// Combines the geometry terms for both view and light directions using the 
// Smith method with Unreal's Schlick-GGX approximation.
// ----------------------------------------------------------------------------
float Unreal_GeometrySmith(float NdotV, float NdotL, float roughness)
{
	float G_V = Unreal_GeometrySchlickGGX(NdotV, roughness);
	float G_L = Unreal_GeometrySchlickGGX(NdotL, roughness);
	return G_V * G_L;
}

// ----------------------------------------------------------------------------
// Average Fresnel Reflectance for Multi-Scattering
//
// Computes an average Fresnel term from the base reflectivity F0. This is used
// in the multiple-scattering approximation (see, e.g., SelfShadow's articles).
// ----------------------------------------------------------------------------
float3 AverageFresnel(float3 F0)
{
	return (F0 * (20.0 / 21.0)) + (1.0 / 21.0);
}

// ----------------------------------------------------------------------------
// Compute Multiple-Scattering Fresnel Term (Fr_ms)
//
// Uses pre-integrated BRDF lookup tables to compute a multi-scattering correction 
// for the specular term. Two LUTs are used:
//   - BRDFLUT: stores data per (Ndot, roughness) with the blue channel holding rsF1 values.
//   - BRDFMSLUT: stores an average rsF1 value in its red channel (sampled at Ndot=0).
//
// The algorithm computes beta factors and combines them with an average Fresnel value.
// ----------------------------------------------------------------------------
float3 ComputeMultiScatteringFresnel(
	Texture2D BRDFLUT,
	Texture2D BRDFMSLUT,
	SamplerState pointSampler,
	float NdotL,
	float NdotV,
	float3 F0,
	float roughness)
{
	// Compute the average Fresnel reflectance from F0.
	float3 fresnelAverage = AverageFresnel(F0);

	// Sample the LUTs:
	float rsF1_avg = BRDFMSLUT.SampleLevel(pointSampler, float2(0.0, roughness), 0).r;
	float rsF1_light = BRDFLUT.SampleLevel(pointSampler, float2(NdotL, roughness), 0).b;
	float rsF1_view = BRDFLUT.SampleLevel(pointSampler, float2(NdotV, roughness), 0).b;

	// Compute beta factors from the LUT values.
	float beta_avg = 1.0 - rsF1_avg;
	float beta_light = 1.0 - rsF1_light;
	float beta_view = 1.0 - rsF1_view;

	// Compute the multi-scattering factor. The denominator uses beta_avg and PI.
	float multiScatterFactor = (beta_light * beta_view) / (beta_avg * PI);

	// Compute an additional Fresnel multiplier term.
	float3 fresnelMultiplier = (fresnelAverage * fresnelAverage * rsF1_avg) /
		max((float3(1.0, 1.0, 1.0) - fresnelAverage * beta_avg), EPSILON);

	return multiScatterFactor * fresnelMultiplier;
}

// ----------------------------------------------------------------------------
// Compute Specular BRDF (Single + Multi-Scattering)
//
// Computes the full specular BRDF as the sum of the single-scattering term and 
// the multi-scattering correction.
// - NdotV: Cosine between surface normal and view direction.
// - NdotL: Cosine between surface normal and light direction.
// - NdotH: Cosine between surface normal and half-vector.
// - LdotH: Cosine between light direction and half-vector.
// - roughness: Surface roughness.
// - F0: Base reflectivity at normal incidence.
// - FresnelTerm: Precomputed Fresnel factor (from Schlick's approximation).
// ----------------------------------------------------------------------------
float3 ComputeSpecularBRDF(
	Texture2D BRDFLUT,
	Texture2D BRDFMSLUT,
	SamplerState pointSampler,
	float NdotV,
	float NdotL,
	float NdotH,
	float LdotH,
	float roughness,
	float3 F0,
	float3 FresnelTerm)
{
	// Single-scattering component: Fresnel * Geometry * Distribution.
	float G = GeometrySmithGGXCorrelated(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 specularSS = FresnelTerm * G * D;

	// Disable multi-scattering component to avoid flickering
	// float3 specularMS = ComputeMultiScatteringFresnel(BRDFLUT, BRDFMSLUT, pointSampler, NdotL, NdotV, F0, roughness);

	return specularSS; // + specularMS;
}

// ----------------------------------------------------------------------------
// Compute Indirect Specular BRDF (Unreal Version)
//
// Uses Unreal Engine's geometry term instead of the correlated Smith method.
// ----------------------------------------------------------------------------
float3 ComputeSpecularBRDF_Indirect(
	Texture2D BRDFLUT,
	Texture2D BRDFMSLUT,
	SamplerState pointSampler,
	float NdotV,
	float NdotL,
	float NdotH,
	float LdotH,
	float roughness,
	float3 F0,
	float3 FresnelTerm)
{
	float G = Unreal_GeometrySmith(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
	float3 specularSS = FresnelTerm * G * D;
	float3 specularMS = ComputeMultiScatteringFresnel(BRDFLUT, BRDFMSLUT, pointSampler, NdotL, NdotV, F0, roughness);
	return specularSS + specularMS;
}

// ----------------------------------------------------------------------------
// Compute Diffuse (BTDF) Term using Disney Diffuse (2015)
//
// Computes the diffuse (or bidirectional transmission) component. For metallic
// surfaces, the diffuse term is reduced. The diffuse term is based on the Disney
// 2015 (Burley) model, scaled by the albedo and 1/PI.
// ----------------------------------------------------------------------------
float3 ComputeDiffuseBRDF(
	float NdotV,
	float NdotL,
	float LdotH,
	float roughness,
	float metallic,
	float3 FresnelTerm,
	float3 albedo)
{
	// kD is the energy remaining after specular reflection (non-metallic surfaces)
	float3 kD = (float3(1.0, 1.0, 1.0) - FresnelTerm) * (1.0 - metallic);
	// Use the Disney 2015 diffuse model (Burley) for light scattering
	float3 diffuse = DisneyDiffuse2015(NdotV, NdotL, LdotH, roughness) * albedo / PI;
	return kD * diffuse;
}

// ----------------------------------------------------------------------------
// Compute Full BSDF
//
// Combines the diffuse and specular components to yield the final BSDF.
// ----------------------------------------------------------------------------
float3 ComputeBSDF(
	Texture2D BRDFLUT,
	Texture2D BRDFMSLUT,
	SamplerState pointSampler,
	float NdotV,
	float NdotL,
	float NdotH,
	float LdotH,
	float roughness,
	float metallic,
	float3 F0,
	float3 FresnelTerm,
	float3 albedo)
{
	float3 diffuse = ComputeDiffuseBRDF(NdotV, NdotL, LdotH, roughness, metallic, FresnelTerm, albedo);
	float3 specular = ComputeSpecularBRDF(BRDFLUT, BRDFMSLUT, pointSampler, NdotV, NdotL, NdotH, LdotH, roughness, F0, FresnelTerm);
	return diffuse + specular;
}

// ----------------------------------------------------------------------------
// Calculate Final Luminance
//
// Computes the final reflected luminance by combining the BSDF with the 
// incident luminous flux and the light cosine term.
// - luminousFlux: The incident light energy (per color channel).
// ----------------------------------------------------------------------------
float3 CalculateLuminance(
	Texture2D BRDFLUT,
	Texture2D BRDFMSLUT,
	SamplerState pointSampler,
	float NdotV,
	float NdotL,
	float NdotH,
	float LdotH,
	float roughness,
	float metallic,
	float3 F0,
	float3 albedo,
	float3 luminousFlux)
{
	// Compute Fresnel factor using Schlick's approximation.
	// Here F90 is assumed to be 1.0 (typical for dielectrics).
	float F90 = 1.0;
	float3 FresnelTerm = Fresnel_Schlick(F0, F90, LdotH);

	// Compute the full BSDF (diffuse + specular).
	float3 bsdf = ComputeBSDF(BRDFLUT, BRDFMSLUT, pointSampler, NdotV, NdotL, NdotH, LdotH, roughness, metallic, F0, FresnelTerm, albedo);

	// Multiply by the incident luminous flux and the cosine term of the light.
	return bsdf * luminousFlux * NdotL;
}
