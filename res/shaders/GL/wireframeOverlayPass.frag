// shadertype=glsl
#version 450
layout(location = 0) in vec4 finalColor;
layout(location = 0) out vec4 uni_debuggerPassRT0;

layout(location = 4) uniform vec4 uni_albedo;
void main()
{
	uni_debuggerPassRT0 = uni_albedo;
}