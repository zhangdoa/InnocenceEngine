// shadertype=glsl
#include "common.glsl"
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

layout(location = 0) uniform mat4 uni_p;
layout(location = 1) uniform mat4 uni_r;
layout(location = 2) uniform mat4 uni_t;
layout(location = 3) uniform mat4 uni_m_local;

void main()
{
	vec4 pos = vec4(in_Position, 1.0);
	gl_Position = uni_p * uni_r * uni_t * uni_m_local * pos;
}