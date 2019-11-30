// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) out VS_OUT
{
	vec4 posWS;
} vs_out;

void main()
{
	// output the fragment position in world space
	vs_out.posWS = perObjectCBuffer.data.m * inPosition;
	gl_Position = GICBuffer.p * GICBuffer.r[0] * GICBuffer.t * vs_out.posWS;
}