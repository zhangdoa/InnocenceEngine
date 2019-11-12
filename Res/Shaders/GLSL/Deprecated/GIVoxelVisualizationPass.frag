// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_RT0;
layout(location = 0) in vec4 voxelColor;

void main()
{
	if (voxelColor.a < 0.1)
	{
		discard;
	}
	uni_RT0 = vec4(voxelColor);
}