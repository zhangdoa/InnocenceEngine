// shadertype=<glsl>
#version 450
layout(location = 0) out vec2 uni_shadowPassRT0;

void main()
{
	uni_shadowPassRT0.r = gl_FragCoord.z;
	uni_shadowPassRT0.g = uni_shadowPassRT0.r * uni_shadowPassRT0.r;
}