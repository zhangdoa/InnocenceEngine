// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) in vec4 finalColor;
layout(location = 0) out vec4 uni_debuggerPassRT0;

void main()
{
	if (finalColor.a == 1.0)
	{
		uni_debuggerPassRT0 = finalColor;
	}
	else
	{
		uni_debuggerPassRT0 = vec4(0.3, 0.4, 0.5, 1.0);
	}
}