// shadertype=glsl
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

layout(location = 0) out vec2 thefrag_TexCoord;

layout(location = 0) uniform mat4 uni_p;
layout(location = 1) uniform mat4 uni_v;
layout(location = 2) uniform mat4 uni_m;

void main()
{
	thefrag_TexCoord = in_TexCoord;
	gl_Position = uni_p * uni_v * uni_m * vec4(in_Position, 1.0);
}