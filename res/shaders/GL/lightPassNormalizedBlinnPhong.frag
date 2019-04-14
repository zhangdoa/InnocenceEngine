// shadertype=glsl
#version 450
layout(location = 0) out vec4 uni_finalColor;

in vec2 TexCoords;

uniform vec3 uni_viewPos;
uniform sampler2D uni_RT0;
uniform sampler2D uni_RT1;
uniform sampler2D uni_RT2;
uniform sampler2D uni_RT3;

struct dirLight {
	vec3 direction;
	vec3 color;
};

struct pointLight {
	vec3 position;
	float radius;
	vec3 color;
};

const float PI = 3.14159265359;
const int NR_POINT_LIGHTS = 64;

uniform dirLight uni_dirLight;

uniform pointLight uni_pointLights[NR_POINT_LIGHTS];

// Oren-Nayar diffuse BRDF
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

vec3 CalcDirectionalLight(dirLight light, vec3 normal, vec3 diffuse, vec3 specular, float roughness, vec3 viewPos, vec3 fragPos)
{
	vec3 N = normalize(normal);
	vec3 L = normalize(-light.direction);
	vec3 V = normalize(viewPos - fragPos);
	vec3 H = normalize(V + L);

	float NdotV = max(dot(N, V), 0.0);
	float LdotV = max(dot(L, V), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	// ambient color
	vec3 ambientColor = diffuse * light.color * 0.04;

	// diffuse color
	float Fd = orenNayarDiffuse(LdotV, NdotL, NdotV, roughness);
	vec3 diffuseColor = diffuse * Fd * light.color;

	// specular color
	float alpha = 32;
	float normalizedScaleFactor = (alpha + 2) / (4 * PI * (2 - pow(2, (-alpha / 2))));
	vec3 specularColor = specular * (1 - Fd) * pow(NdotH, alpha) * normalizedScaleFactor * light.color;

	return (ambientColor + diffuseColor + specularColor);
}

vec3 CalcPointLight(pointLight light, vec3 normal, vec3 diffuse, vec3 specular, float roughness, vec3 viewPos, vec3 fragPos)
{
	if (length(light.position - fragPos) < light.radius)
	{
		vec3 N = normalize(normal);
		vec3 L = normalize(light.position - fragPos);;
		vec3 V = normalize(viewPos - fragPos);
		vec3 H = normalize(V + L);

		float NdotV = max(dot(N, V), 0.0);
		float LdotV = max(dot(L, V), 0.0);
		float NdotH = max(dot(N, H), 0.0);
		float NdotL = max(dot(N, L), 0.0);

		// ambient color
		vec3 ambientColor = diffuse * light.color * 0.04;

		// diffuse color
		float Fd = orenNayarDiffuse(LdotV, NdotL, NdotV, roughness);
		vec3 diffuseColor = diffuse * Fd * light.color;

		// specular color
		float alpha = 32;
		float normalizedScaleFactor = (alpha + 2) / (4 * PI * (2 - pow(2, (-alpha / 2))));
		vec3 specularColor = specular * (1 - Fd) * pow(NdotH, alpha) * normalizedScaleFactor * light.color;

		//attenuation
		float distance = length(light.position - fragPos);
		float attenuation = 1.0 / (distance * distance);

		ambientColor *= attenuation;
		diffuseColor *= attenuation;
		specularColor *= attenuation;

		return (ambientColor + diffuseColor + specularColor);
	}
	else
	{
		return vec3(0.0f, 0.0f, 0.0f);
	}
}

void main()
{
	if (texture(uni_RT0, TexCoords).a == 1.0)
	{
		uni_finalColor.a = 0.0;
	}
	else
	{
		// retrieve data from gbuffer
		vec3 FragPos = texture(uni_RT0, TexCoords).rgb;
		vec3 Normal = texture(uni_RT1, TexCoords).rgb;
		vec3 Diffuse = texture(uni_RT2, TexCoords).rgb;
		vec3 Specular = texture(uni_RT3, TexCoords).rgb;

		//final result
		uni_finalColor.rgb = CalcDirectionalLight(uni_dirLight, Normal, Diffuse, Specular, 0.0, uni_viewPos, FragPos);

		for (int i = 0; i < NR_POINT_LIGHTS; i++)
		{
			uni_finalColor.rgb += CalcPointLight(uni_pointLights[i], Normal, Diffuse, Specular, 0.0, uni_viewPos, FragPos);
		}
		uni_finalColor.a = 1.0;
	}
}