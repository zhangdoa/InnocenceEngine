// shadertype=glsl
#version 450
layout(location = 0) out vec4 uni_terrainPassRT0;

void main()
{
	vec3 albedo = vec3(0.5f, 0.3f, 0.7f);

	uni_terrainPassRT0 = vec4(albedo, 1.0f);
}