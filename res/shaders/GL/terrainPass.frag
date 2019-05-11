// shadertype=glsl
#version 450

layout(location = 0) out vec4 uni_terrainPassRT0;

in GS_OUT
{
	vec4 normal;
} fs_in;

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
	vec3 N = normalize(fs_in.normal.xyz);
	vec3 L = normalize(-uni_dirLight.direction.xyz);
	float NdotL = max(dot(N, L), 0.0);

	uni_terrainPassRT0 = vec4(fs_in.normal.xyz, 1.0);
}