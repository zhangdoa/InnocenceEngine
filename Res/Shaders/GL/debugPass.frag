// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) out vec4 uni_debugPassRT0;

void main()
{
	uni_debugPassRT0 = vec4(0.2, 0.3, 0.4, 1.0);
}