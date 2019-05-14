// shadertype=glsl
#version 450
#extension GL_ARB_shader_image_load_store : require
#define BLOCK_SIZE 16

layout(location = 0) in vec2 TexCoords;

layout(location = 0) out vec4 uni_lightPassRT0;

struct dirLight {
	vec4 direction;
	vec4 luminance;
	mat4 r;
};

// w component of luminance is attenuationRadius
struct pointLight {
	vec4 position;
	vec4 luminance;
	//float attenuationRadius;
};

// w component of luminance is sphereRadius
struct sphereLight {
	vec4 position;
	vec4 luminance;
	//float sphereRadius;
};

struct CSM {
	mat4 p;
	mat4 v;
	vec4 splitCorners;
};

const float eps = 0.00001;
const float PI = 3.14159265359;
const int NR_POINT_LIGHTS = 1024;
const int NR_SPHERE_LIGHTS = 128;
const int NR_CSM_SPLITS = 4;
const float MAX_REFLECTION_LOD = 4.0;

layout(location = 0, binding = 0) uniform sampler2D uni_opaquePassRT0;
layout(location = 1, binding = 1) uniform sampler2D uni_opaquePassRT1;
layout(location = 2, binding = 2) uniform sampler2D uni_opaquePassRT2;
layout(location = 3, binding = 3) uniform sampler2D uni_SSAOBlurPassRT0;
layout(location = 4, binding = 4) uniform sampler2D uni_directionalLightShadowMap;
layout(location = 5, binding = 5) uniform sampler2D uni_brdfLUT;
layout(location = 6, binding = 6) uniform sampler2D uni_brdfMSLUT;
layout(location = 7, binding = 7) uniform samplerCube uni_irradianceMap;
layout(location = 8, binding = 8) uniform samplerCube uni_preFiltedMap;
layout(binding = 0, rgba16f) uniform image2D uni_lightGrid;

layout(std140, row_major, binding = 0) uniform cameraUBO
{
	mat4 uni_p_camera_original;
	mat4 uni_p_camera_jittered;
	mat4 uni_r_camera;
	mat4 uni_t_camera;
	mat4 uni_r_camera_prev;
	mat4 uni_t_camera_prev;
	vec4 uni_globalPos;
	float WHRatio;
};

layout(std140, row_major, binding = 3) uniform sunUBO
{
	dirLight uni_dirLight;
};

layout(std140, row_major, binding = 4) uniform pointLightUBO
{
	pointLight uni_pointLights[NR_POINT_LIGHTS];
};

layout(std140, row_major, binding = 5) uniform sphereLightUBO
{
	sphereLight uni_sphereLights[NR_SPHERE_LIGHTS];
};

layout(std140, row_major, binding = 6) uniform CSMUBO
{
	CSM uni_CSMs[NR_CSM_SPLITS];
};

layout(std430, binding = 2) buffer lightIndexListSSBO
{
	uint lightIndexList[];
};

bool uni_drawCSMSplitedArea = false;

uvec2 RGBA16F2RG32UI(vec4 rhs)
{
	uint x = (uint(rhs.x) & 0x0000FFFF) | (uint(rhs.y) & 0x0000FFFF << 16U);
	uint y = (uint(rhs.z) & 0x0000FFFF) | (uint(rhs.w) & 0x0000FFFF << 16U);
	return uvec2(x, y);
}

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
vec3 getFrMS(float NdotL, float NdotV, vec3 F0, float roughness)
{
	float alpha = roughness * roughness;
	vec3 f_averange = AverangeFresnel(F0);
	float rsF1_averange = texture(uni_brdfMSLUT, vec2(0.0, alpha)).r;
	float rsF1_l = texture(uni_brdfLUT, vec2(NdotL, alpha)).b;
	float rsF1_v = texture(uni_brdfLUT, vec2(NdotV, alpha)).b;

	vec3 frMS = vec3(0.0);
	float beta1 = 1.0 - rsF1_averange;
	float beta2 = 1.0 - rsF1_l;
	float beta3 = 1.0 - rsF1_v;

	frMS = f_averange * rsF1_averange / (PI * beta1 * (vec3(1.0) - f_averange * beta1) + eps);
	frMS = frMS * beta2 * beta3;
	return frMS;
}
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
	vec3 Frms = getFrMS(NdotL, NdotV, F0, roughness);

	vec3 Fr = Frss + Frms;

	// Diffuse BRDF
	vec3 Fd = fd_DisneyDiffuse(NdotV, NdotL, LdotH, roughness * roughness) * Albedo;

	return (Fd + Fr) * lightLuminance * NdotL;
}
// IBL
// corrected Fresnel
// ----------------------------------------------------------------------------
vec3 fr_F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------
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
	vec3 Frms = getFrMS(NdotL, NdotV, F0, roughness);
	vec3 specular = prefilteredColor * ((F * Frss.x + Frss.y) + Frms);

	return (kD * diffuse + specular);
}
// shadow mapping
// ----------------------------------------------------------------------------
float PCF(float NdotL, vec3 projCoords, sampler2D shadowMap, vec2 texelSize, vec2 offset)
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
float ShadowCalculation(float NdotL, vec3 fragPos)
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
		if (fragPos.x >= uni_CSMs[i].splitCorners.x &&
			fragPos.z >= uni_CSMs[i].splitCorners.y &&
			fragPos.x <= uni_CSMs[i].splitCorners.z &&
			fragPos.z <= uni_CSMs[i].splitCorners.w)
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

	shadow = PCF(NdotL, projCoords, uni_directionalLightShadowMap, texelSize, offset);

	return shadow;
}
// ----------------------------------------------------------------------------
void main()
{
	vec4 RT0 = texture(uni_opaquePassRT0, TexCoords);
	vec4 RT1 = texture(uni_opaquePassRT1, TexCoords);
	vec4 RT2 = texture(uni_opaquePassRT2, TexCoords);

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

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, Albedo, Metallic);

	vec3 N = normalize(Normal);
	vec3 V = normalize(uni_globalPos.xyz - FragPos);

	float NdotV = max(dot(N, V), 0.0);

	// direction light, sun light
	vec3 L = normalize(-uni_dirLight.direction.xyz);
	vec3 H = normalize(V + L);

	float LdotH = max(dot(L, H), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, safe_roughness, F0, Albedo, uni_dirLight.luminance.xyz);

	Lo *= 1 - ShadowCalculation(NdotL, FragPos);

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
	vec3 R = reflect(-V, N);
	Lo += imageBasedLight(N, NdotV, R, Albedo, Metallic, safe_roughness, F0);

	// ambient occlusion
	Lo *= AO;

	if (uni_drawCSMSplitedArea)
	{
		vec3 N = normalize(Normal);

		vec3 L = normalize(-uni_dirLight.direction.xyz);
		float NdotL = max(dot(N, L), 0.0);

		Lo = vec3(NdotL);

		Lo *= 1 - ShadowCalculation(NdotL, FragPos);

		int splitIndex = NR_CSM_SPLITS;
		for (int i = 0; i < NR_CSM_SPLITS; i++)
		{
			if (FragPos.x >= uni_CSMs[i].splitCorners.x &&
				FragPos.z >= uni_CSMs[i].splitCorners.y &&
				FragPos.x <= uni_CSMs[i].splitCorners.z &&
				FragPos.z <= uni_CSMs[i].splitCorners.w)
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
	}

	uni_lightPassRT0.rgb = Lo;
	uni_lightPassRT0.a = 1.0;
}