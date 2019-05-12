// shadertype=glsl
#version 450
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec3 TexCoords;

const float PI = 3.14159265359;

struct dirLight {
	vec4 direction;
	vec4 luminance;
	mat4 r;
};

layout(std140, row_major, binding = 3) uniform sunUBO
{
	dirLight uni_dirLight;
};

layout(location = 0, binding = 0) uniform samplerCube uni_opaquePassRT0;
layout(location = 1, binding = 1) uniform samplerCube uni_opaquePassRT1;
layout(location = 2, binding = 2) uniform samplerCube uni_opaquePassRT2;

void main()
{
	vec3 N = texture(uni_opaquePassRT1, TexCoords).rgb;
	N = normalize(N);

	vec3 L = -uni_dirLight.direction.xyz;
	L = normalize(L);

	float NdotL = max(dot(N, L), 0.0);
	vec3 albedo = texture(uni_opaquePassRT2, TexCoords).rgb;

	albedo *= NdotL / PI;

	FragColor = vec4(albedo, 1.0);
}