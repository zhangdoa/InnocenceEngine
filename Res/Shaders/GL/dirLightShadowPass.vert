// shadertype=glsl
#include "common.glsl"

layout(location = 0) in vec3 in_Position;

layout(location = 0) uniform mat4 uni_p;
layout(location = 1) uniform mat4 uni_v;
layout(location = 2) uniform mat4 uni_m_local;

void main()
{
	gl_Position = uni_p * uni_v * uni_m_local * vec4(in_Position, 1.0);
}