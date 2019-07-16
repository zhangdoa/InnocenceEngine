// shadertype=glsl
#include "common.glsl"
layout(location = 0) out vec4 uni_RT0;
layout(location = 0) in vec4 voxelColor;

void main()
{
	uni_RT0 = vec4(voxelColor);
}