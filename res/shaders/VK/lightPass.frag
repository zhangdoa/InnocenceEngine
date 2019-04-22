#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 outTexCoord;

layout(location = 0) out vec4 lightPassRT0;

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

const float eps = 0.00001;
const float PI = 3.14159265359;
const int NR_POINT_LIGHTS = 64;
const int NR_SPHERE_LIGHTS = 64;

const float MAX_REFLECTION_LOD = 4.0;

layout(std140, row_major, set = 0, binding = 0) uniform cameraUBO
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

layout(set = 0, binding = 1) uniform sunUBO
{
	dirLight uni_dirLight;
};

layout(set = 0, binding = 2) uniform pointLightUBO
{
	pointLight uni_pointLights[NR_POINT_LIGHTS];
};

layout(set = 0, binding = 3) uniform sphereLightUBO
{
	sphereLight uni_sphereLights[NR_SPHERE_LIGHTS];
};

layout(set = 0, binding = 4) uniform sampler2D uni_opaquePassRT0;
layout(set = 0, binding = 5) uniform sampler2D uni_opaquePassRT1;
layout(set = 0, binding = 6) uniform sampler2D uni_opaquePassRT2;

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
	float	NdotL_ = max(NdotL, eps);
	float energyBias = mix(0.0, 0.5, linearRoughness);
	float energyFactor = mix(1.0, 1.0 / 1.51, linearRoughness);
	float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
	vec3 f0 = vec3(1.0, 1.0, 1.0);
	float lightScatter = fr_F_Schlick(f0, fd90, NdotL_).r;
	float viewScatter = fr_F_Schlick(f0, fd90, NdotV_).r;
	return lightScatter * viewScatter * energyFactor;
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
// ----------------------------------------------------------------------------
vec3 getIlluminance(float NdotV, float LdotH, float NdotH, float NdotL, float roughness, vec3 F0, vec3 Albedo, vec3 lightLuminance)
{
	// Specular BRDF
	float f90 = 1.0;
	vec3 F = fr_F_Schlick(F0, f90, NdotV);
	float G = fr_V_SmithGGXCorrelated(NdotV, NdotL, roughness);
	float D = fr_D_GGX(NdotH, roughness);
	vec3 Frss = F * D * G / PI;

	vec3 Fr = Frss;

	// Diffuse BRDF
	vec3 Fd = fd_DisneyDiffuse(NdotV, NdotL, LdotH, roughness * roughness) * Albedo / PI;

	return (Fd + Fr) * lightLuminance * NdotL;
}
// ----------------------------------------------------------------------------
void main()
{
	vec4 RT0 = texture(uni_opaquePassRT0, outTexCoord);
	vec4 RT1 = texture(uni_opaquePassRT1, outTexCoord);
	vec4 RT2 = texture(uni_opaquePassRT2, outTexCoord);

	vec3 FragPos = RT0.rgb;
	vec3 Normal = RT1.rgb;
	vec3 Albedo = RT2.rgb;

	float Metallic = RT0.a;
	float Roughness = RT1.a;
	float safe_roughness = (Roughness + eps) / (1.0 + eps);
	float AO = RT2.a;

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

	// point punctual light
	for (int i = 0; i < NR_POINT_LIGHTS; ++i)
	{
		vec3 unormalizedL = uni_pointLights[i].position.xyz - FragPos;
		float lightRadius = uni_pointLights[i].luminance.w;
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

			vec3 lightLuminance = uni_pointLights[i].luminance.xyz * attenuation;

			Lo += getIlluminance(NdotV, LdotH, NdotH, NdotL, safe_roughness, F0, Albedo, lightLuminance);
		}
	}

	// sphere area light
	for (int i = 0; i < NR_SPHERE_LIGHTS; ++i)
	{
		vec3 unormalizedL = uni_sphereLights[i].position.xyz - FragPos;
		float lightRadius = uni_sphereLights[i].luminance.w;

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

	// ambient occlusion
	Lo *= AO;

	lightPassRT0.rgb = Lo;
	lightPassRT0.a = 1.0;
}