// shadertype=glsl
#include "common.glsl"
#include "BRDF.glsl"
#include "utility.glsl"

layout(location = 0) in vec2 TexCoords;

layout(location = 0) out vec4 uni_lightPassRT0;

layout(location = 0, binding = 0) uniform sampler2D uni_opaquePassRT0;
layout(location = 1, binding = 1) uniform sampler2D uni_opaquePassRT1;
layout(location = 2, binding = 2) uniform sampler2D uni_opaquePassRT2;
layout(location = 3, binding = 3) uniform sampler2D uni_opaquePassRT3;
layout(location = 4, binding = 4) uniform sampler2D uni_SSAOBlurPassRT0;
layout(location = 5, binding = 5) uniform sampler2D uni_directionalLightShadowMap;
layout(location = 6, binding = 6) uniform samplerCube uni_pointLightShadowMap;
layout(location = 7, binding = 7) uniform sampler2D uni_brdfLUT;
layout(location = 8, binding = 8) uniform sampler2D uni_brdfMSLUT;
layout(location = 9, binding = 9) uniform samplerCube uni_irradianceMap;
layout(location = 10, binding = 10) uniform samplerCube uni_preFiltedMap;
layout(location = 11, binding = 11) uniform sampler2D uni_depth;

layout(binding = 0, rgba16f) uniform image2D uni_lightGrid;

// ----------------------------------------------------------------------------
vec3 getIlluminance(float NdotV, float LdotH, float NdotH, float NdotL, float roughness, vec3 F0, vec3 Albedo, vec3 lightLuminance)
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
	vec3 Fd = fd_DisneyDiffuse(NdotV, NdotL, LdotH, roughness * roughness) * Albedo;

	return (Fd + Fr) * lightLuminance * NdotL;
}
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
// shadow mapping
// ----------------------------------------------------------------------------
float PCF(vec3 projCoords, sampler2D shadowMap, vec2 texelSize, vec2 offset)
{
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;

	// PCF
	float shadow = 0.0;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowMap, offset + projCoords.xy / 2.0 + vec2(x, y) * texelSize).r;
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	return shadow;
}
// ----------------------------------------------------------------------------
float VSMKernel(vec4 shadowMapValue, float currentDepth)
{
	float shadow = 0.0;
	float Ex = shadowMapValue.r;
	float E_x2 = shadowMapValue.g;
	float variance = E_x2 - (Ex * Ex);
	float mD = Ex - currentDepth;
	float p = variance / (variance + mD * mD);

	shadow = max(p, float(currentDepth >= Ex));

	return shadow;
}
// ----------------------------------------------------------------------------
float DirectionalLightVSM(float NdotL, vec3 projCoords, sampler2D shadowMap, vec2 texelSize, vec2 offset)
{
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;

	// VSM
	vec4 shadowMapValue = texture(shadowMap, offset + projCoords.xy / 2.0);
	float shadow = VSMKernel(shadowMapValue, currentDepth);
	return shadow;
}
// ----------------------------------------------------------------------------
float DirectionalLightShadow(vec3 fragPos)
{
	vec3 projCoords = vec3(0.0);
	float shadow = 0.0;
	vec2 textureSize = textureSize(uni_directionalLightShadowMap, 0);
	vec2 texelSize = 1.0 / textureSize;
	// one shadow texture stores 4 different split results in character "N" shape order from left-bottom corner
	// 1 -- 3
	// |    |
	// 0 -- 2
	vec2 offsetSize = vec2(0.5, 0.5);

	int splitIndex = NR_CSM_SPLITS;
	for (int i = 0; i < NR_CSM_SPLITS; i++)
	{
		if (fragPos.x >= uni_CSMs[i].AABBMin.x &&
			fragPos.y >= uni_CSMs[i].AABBMin.y &&
			fragPos.z >= uni_CSMs[i].AABBMin.z &&
			fragPos.x <= uni_CSMs[i].AABBMax.x &&
			fragPos.y <= uni_CSMs[i].AABBMax.y &&
			fragPos.z <= uni_CSMs[i].AABBMax.z)
		{
			splitIndex = i;
			break;
		}
	}

	vec2 offset;

	if (splitIndex == NR_CSM_SPLITS)
	{
		shadow = 0.0;
	}
	else
	{
		vec4 lightSpacePos = uni_CSMs[splitIndex].p * uni_CSMs[splitIndex].v * vec4(fragPos, 1.0f);
		lightSpacePos = lightSpacePos / lightSpacePos.w;
		projCoords = lightSpacePos.xyz;

		if (splitIndex == 0)
		{
			offset = vec2(0, 0);
		}
		else if (splitIndex == 1)
		{
			offset = vec2(0, offsetSize.y);
		}
		else if (splitIndex == 2)
		{
			offset = vec2(offsetSize.x, 0);
		}
		else if (splitIndex == 3)
		{
			offset = offsetSize;
		}
		else
		{
			shadow = 0.0;
		}
	}

	shadow = PCF(projCoords, uni_directionalLightShadowMap, texelSize, offset);

	return shadow;
}
// ----------------------------------------------------------------------------
float PointLightShadow(vec3 fragPos)
{
	vec3 fragToLight = fragPos - uni_pointLights[0].position.xyz;
	float currentDepth = length(fragToLight);
	float lightRadius = uni_pointLights[0].luminance.w;

	if (currentDepth > lightRadius)
	{
		return 0;
	}

	vec3 texCoord = normalize(fragToLight);
	vec4 shadowMapValue = texture(uni_pointLightShadowMap, texCoord);

	float shadow = VSMKernel(shadowMapValue, currentDepth);
	return shadow;
}
// ----------------------------------------------------------------------------
float linearDepth(float depthSample)
{
	float zLinear = zNear * zFar / (zFar + zNear - depthSample * (zFar - zNear));
	return zLinear;
}
// ----------------------------------------------------------------------------
void main()
{
	vec4 RT0 = texture(uni_opaquePassRT0, TexCoords);
	vec4 RT1 = texture(uni_opaquePassRT1, TexCoords);
	vec4 RT2 = texture(uni_opaquePassRT2, TexCoords);
	vec4 RT3 = texture(uni_opaquePassRT3, TexCoords);

	//vec2 textureSize = textureSize(uni_depth, 0);
	//vec2 screenTexCoord = gl_FragCoord.xy / textureSize;
	//float depth = texture(uni_depth, screenTexCoord).r;
	//vec4 posCS = vec4(screenTexCoord.x * 2.0f - 1.0f, screenTexCoord.y * 2.0f - 1.0f, depth * 2.0f - 1.0f, 1.0f);
	//vec4 posVS = uni_p_inv * posCS;
	//posVS /= posVS.w;
	//vec4 posWS = uni_v_inv * posVS;
	//vec3 FragPos = posWS.rgb;

	vec3 FragPos = RT0.rgb;
	vec3 Normal = RT1.rgb;
	vec3 Albedo = RT2.rgb;

	float Metallic = RT0.a;
	float Roughness = RT1.a;
	float safe_roughness = (Roughness + eps) / (1.0 + eps);
	float AO = RT2.a;
	float SSAO = texture(uni_SSAOBlurPassRT0, TexCoords).x;
	AO *= pow(SSAO, 2.0f);

	vec3 Lo = vec3(0.0);
	vec3 N = normalize(Normal);
	vec3 L;
	float NdotL;

#ifdef uni_drawCSMSplitedArea
	L = normalize(-uni_dirLight.direction.xyz);
	NdotL = max(dot(N, L), 0.0);

	Lo = vec3(NdotL);
	Lo *= 1 - DirectionalLightShadow(FragPos);

	int splitIndex = NR_CSM_SPLITS;
	for (int i = 0; i < NR_CSM_SPLITS; i++)
	{
		if (FragPos.x >= uni_CSMs[i].AABBMin.x &&
			FragPos.y >= uni_CSMs[i].AABBMin.y &&
			FragPos.z >= uni_CSMs[i].AABBMin.z &&
			FragPos.x <= uni_CSMs[i].AABBMax.x &&
			FragPos.y <= uni_CSMs[i].AABBMax.y &&
			FragPos.z <= uni_CSMs[i].AABBMax.z)
		{
			splitIndex = i;
			break;
		}
	}

	if (splitIndex == 0)
	{
		Lo.g = 0;
		Lo.b = 0;
	}
	else if (splitIndex == 1)
	{
		Lo.b = 0;
	}
	else if (splitIndex == 2)
	{
		Lo.r = 0;
		Lo.b = 0;
	}
	else if (splitIndex == 3)
	{
		Lo.r = 0;
		Lo.g = 0;
	}
#endif

#ifdef uni_drawPointLightShadow
	pointLight light = uni_pointLights[0];

	float lightRadius = light.luminance.w;
	if (lightRadius > 0)
	{
		vec3 unormalizedL = light.position.xyz - FragPos;

		if (length(unormalizedL) < lightRadius)
		{
			L = normalize(unormalizedL);
			NdotL = max(dot(N, L), 0.0);

			float attenuation = 1.0;
			float invSqrAttRadius = 1.0 / max(lightRadius * lightRadius, eps);
			attenuation *= getDistanceAtt(unormalizedL, invSqrAttRadius);

			vec3 lightLuminance = light.luminance.xyz * vec3(NdotL) * attenuation;

			Lo = lightLuminance;
		}
	}
	Lo *= 1 - PointLightShadow(FragPos);
#endif
#if !defined (uni_drawCSMSplitedArea) && !defined (uni_drawPointLightShadow)
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, Albedo, Metallic);

	vec3 V = normalize(uni_globalPos.xyz - FragPos);

	float NdotV = max(dot(N, V), 0.0);

	// direction light, sun light
	L = normalize(-uni_dirLight.direction.xyz);
	vec3 H = normalize(V + L);

	float LdotH = max(dot(L, H), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	NdotL = max(dot(N, L), 0.0);

	Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, safe_roughness, F0, Albedo, uni_dirLight.luminance.xyz);

	Lo *= 1 - DirectionalLightShadow(FragPos);

	// point punctual light
	// Get the index of the current pixel in the light grid.
	ivec2 tileIndex = ivec2(floor(gl_FragCoord.xy / BLOCK_SIZE));

	// Get the start position and offset of the light in the light index list.
	vec4 lightGridRGBA16F = imageLoad(uni_lightGrid, tileIndex);
	uvec2 lightGrid = RGBA16F2RG32UI(lightGridRGBA16F);
	uint startOffset = lightGrid.x;
	uint lightCount = lightGrid.y;

	//for (int i = 0; i < lightCount; ++i)
	for (int i = 0; i < NR_POINT_LIGHTS; ++i)
	{
		//uint lightIndex = lightIndexList[startOffset + i];
		uint lightIndex = i;
		pointLight light = uni_pointLights[lightIndex];

		float lightRadius = light.luminance.w;
		if (lightRadius > 0)
		{
			vec3 unormalizedL = light.position.xyz - FragPos;

			if (length(unormalizedL) < lightRadius)
			{
				L = normalize(unormalizedL);
				H = normalize(V + L);

				LdotH = max(dot(L, H), 0.0);
				NdotH = max(dot(N, H), 0.0);
				NdotL = max(dot(N, L), 0.0);

				float attenuation = 1.0;
				float invSqrAttRadius = 1.0 / max(lightRadius * lightRadius, eps);
				attenuation *= getDistanceAtt(unormalizedL, invSqrAttRadius);

				vec3 lightLuminance = light.luminance.xyz * attenuation;

				Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, safe_roughness, F0, Albedo, lightLuminance);
			}
		}
	}

	Lo *= 1 - PointLightShadow(FragPos);

	// sphere area light
	for (int i = 0; i < NR_SPHERE_LIGHTS; ++i)
	{
		float lightRadius = uni_sphereLights[i].luminance.w;
		if (lightRadius > 0)
		{
			vec3 unormalizedL = uni_sphereLights[i].position.xyz - FragPos;
			L = normalize(unormalizedL);
			H = normalize(V + L);

			LdotH = max(dot(L, H), 0.0);
			NdotH = max(dot(N, H), 0.0);
			NdotL = max(dot(N, L), 0.0);

			float sqrDist = dot(unormalizedL, unormalizedL);

			float Beta = acos(NdotL);
			float H2 = sqrt(sqrDist);
			float h = H2 / lightRadius;
			float x = sqrt(max(h * h - 1, eps));
			float y = -x * (1 / tan(Beta));
			y = clamp(y, -1.0, 1.0);
			float illuminance = 0;

			if (h * cos(Beta) > 1)
			{
				illuminance = cos(Beta) / (h * h);
			}
			else
			{
				illuminance = (1 / max(PI * h * h, eps))
					* (cos(Beta) * acos(y) - x * sin(Beta) * sqrt(max(1 - y * y, eps)))
					+ (1 / PI) * atan((sin(Beta) * sqrt(max(1 - y * y, eps)) / x));
			}
			illuminance *= PI;

			Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, safe_roughness, F0, Albedo, illuminance * uni_sphereLights[i].luminance.xyz);
		}
	}

	// environment capture light
	// Dynamic object
	if (int(RT3.a) == 1)
	{
		vec3 R = reflect(-V, N);
		Lo += imageBasedLight(N, NdotV, R, Albedo, Metallic, safe_roughness, F0);
	}

	// ambient occlusion
	Lo *= AO;
#endif
	uni_lightPassRT0.rgb = Lo;
	uni_lightPassRT0.a = 1.0;
	//uni_lightPassRT0 = posWS;
}