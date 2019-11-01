// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec2 uni_shadowPassRT0;

layout(location = 0) in vec4 FragPos;

void main()
{
	float lightDistance = length(FragPos.xyz - pointLightUBO.data[0].position.xyz);
	uni_shadowPassRT0.r = lightDistance;
	uni_shadowPassRT0.g = uni_shadowPassRT0.r * uni_shadowPassRT0.r;
}