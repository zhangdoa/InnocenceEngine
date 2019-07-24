// shadertype=glsl
#include "common.glsl"
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inPad1;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inPad2;

layout(location = 0) uniform mat4 uni_p;
layout(location = 1) uniform mat4 uni_r;
layout(location = 2) uniform mat4 uni_t;
layout(location = 3) uniform mat4 uni_m_local;

void main()
{
	gl_Position = uni_p * uni_r * uni_t * uni_m_local * inPosition;
}