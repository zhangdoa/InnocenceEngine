// shadertype=glsl
#version 450

layout(location = 0) in GS_OUT
{
	vec4 normal;
}fs_in;

layout(location = 0) out vec4 uni_terrainPassRT0;

struct dirLight {
	vec4 direction;
	vec4 luminance;
	mat4 r;
};

layout(std140, row_major, binding = 3) uniform sunUBO
{
	dirLight uni_dirLight;
};

void main()
{
	vec3 N = fs_in.normal.xyz;
	N = N * 2.0 - 1.0;
	N = normalize(N);

	vec3 L = normalize(-uni_dirLight.direction.xyz);
	float NdotL = max(dot(N, L), 0.0);
	vec3 finalColor = vec3(NdotL, NdotL, NdotL);

	//uni_terrainPassRT0 = vec4(N, 1.0);

	uni_terrainPassRT0 = vec4(finalColor, 1.0);
}