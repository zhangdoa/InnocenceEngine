// shadertype=glsl
#include "common.glsl"
layout(location = 0) in vec4 finalColor;
layout(location = 0) out vec4 uni_debuggerPassRT0;

layout(location = 4) uniform vec4 uni_albedo_local;
void main()
{
	uni_debuggerPassRT0 = uni_albedo_local;
}