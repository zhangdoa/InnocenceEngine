// shadertype=glsl
#version 450

layout(vertices = 16) out;

void main()
{
	gl_TessLevelInner[0] = 4;
	gl_TessLevelInner[1] = 4;

	gl_TessLevelOuter[0] = 4;
	gl_TessLevelOuter[1] = 4;
	gl_TessLevelOuter[2] = 4;
	gl_TessLevelOuter[3] = 4;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}