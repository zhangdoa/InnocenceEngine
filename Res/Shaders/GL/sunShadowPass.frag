// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec2 uni_sunShadowPassRT0;

void main()
{
	uni_sunShadowPassRT0.r = gl_FragCoord.z;
	uni_sunShadowPassRT0.g = uni_sunShadowPassRT0.r * uni_sunShadowPassRT0.r;
}