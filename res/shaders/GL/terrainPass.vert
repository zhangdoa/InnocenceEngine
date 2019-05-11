// shadertype=glsl
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

void main()
{
	gl_Position = vec4(in_Position, 1.0f);
}