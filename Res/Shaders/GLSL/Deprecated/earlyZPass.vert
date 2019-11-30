// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

void main()
{
	vec4 posWS = perObjectCBuffer.data.m * inPosition;

	vec4 posVS = perFrameCBuffer.data.v * posWS;

	gl_Position = perFrameCBuffer.data.p_jittered * posVS;
}