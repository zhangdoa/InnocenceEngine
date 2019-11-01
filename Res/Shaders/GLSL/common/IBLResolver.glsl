// ----------------------------------------------------------------------------
const float MAX_REFLECTION_LOD = 4.0;
vec3 imageBasedLight(vec3 N, float NdotV, vec3 R, vec3 albedo, float metallic, float roughness, vec3 F0)
{
	vec3 F = fr_F_SchlickRoughness(NdotV, F0, roughness);
	vec3 kS = fr_F_Schlick(F0, 1.0, NdotV);
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;
	vec3 irradiance = texture(uni_irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
	vec3 L = R;
	float NdotL = max(dot(N, L), 0.0);
	vec3 prefilteredColor = textureLod(uni_preFiltedMap, L, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 Frss = texture(uni_brdfLUT, vec2(NdotV, roughness)).rg;
	vec3 Frms = getFrMS(uni_brdfLUT, uni_brdfMSLUT, NdotL, NdotV, F0, roughness);
	vec3 specular = prefilteredColor * ((F * Frss.x + Frss.y) + Frms);

	return (kD * diffuse + specular);
}