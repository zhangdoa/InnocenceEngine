// shadertype=glsl
#include "common.glsl"
layout(location = 0) in vec3 in_Position;

layout(location = 0) out vec3 TexCoords;

layout(location = 0) uniform mat4 uni_p;
layout(location = 1) uniform mat4 uni_r;

void main()
{
	TexCoords = in_Position;
	gl_Position = uni_p * uni_r * vec4(in_Position, 1.0);
}