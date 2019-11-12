// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

void main()
{
	vec4 posWS = meshUBO.m * inPosition;

	vec4 posVS = cameraUBO.r * cameraUBO.t * posWS;

	gl_Position = cameraUBO.p_jittered * posVS;
}