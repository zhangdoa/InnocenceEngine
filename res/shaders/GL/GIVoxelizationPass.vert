// shadertype=glsl
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

uniform mat4 uni_m;

out VS_OUT{
	vec2 texCoord;
	vec3 normal;
} vs_out;

void main()
{
	vec4 thefrag_WorldSpacePos = uni_m * vec4(in_Position, 1.0);

	vs_out.normal = mat3(transpose(inverse(uni_m))) * in_Normal;
	vs_out.texCoord = in_TexCoord;

	gl_Position = thefrag_WorldSpacePos;
}