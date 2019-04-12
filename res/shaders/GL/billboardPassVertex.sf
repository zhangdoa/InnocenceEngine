// shadertype=<glsl>
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;

out vec2 thefrag_TexCoord;

layout(location = 0) uniform mat4 uni_p;
layout(location = 1) uniform mat4 uni_r;
layout(location = 2) uniform mat4 uni_t;
layout(location = 3) uniform vec3 uni_pos;
layout(location = 4) uniform vec2 uni_size;

void main()
{
	gl_Position = uni_p * uni_r * uni_t * vec4(uni_pos, 1.0);
	gl_Position /= gl_Position.w;
	gl_Position.xy += in_Position.xy * uni_size;
	thefrag_TexCoord = in_TexCoord;
}