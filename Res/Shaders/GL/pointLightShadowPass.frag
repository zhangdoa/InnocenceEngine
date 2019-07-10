// shadertype=<glsl>
#version 450
layout(location = 0) out vec2 uni_shadowPassRT0;

layout(location = 0) in vec4 FragPos;

const int NR_POINT_LIGHTS = 1024;

// w component of luminance is attenuationRadius
struct pointLight {
	vec4 position;
	vec4 luminance;
	//float attenuationRadius;
};

layout(std140, row_major, binding = 4) uniform pointLightUBO
{
	pointLight uni_pointLights[NR_POINT_LIGHTS];
};

void main()
{
	float lightDistance = length(FragPos.xyz - uni_pointLights[0].position.xyz);
	uni_shadowPassRT0.r = lightDistance;
	uni_shadowPassRT0.g = uni_shadowPassRT0.r * uni_shadowPassRT0.r;
}