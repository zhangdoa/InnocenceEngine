// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_SHVisualization;
layout(location = 0) in vec3 thefrag_Normal;

const float A0 = 3.141593;
const float A1 = 2.094395;
const float A2 = 0.785398;

layout(location = 1) uniform uint uni_SH9Index;

// ----------------------------------------------------------------------------
void main()
{
	float Y00 = 0.282095;
	float Y11 = 0.488603 * thefrag_Normal.x;
	float Y10 = 0.488603 * thefrag_Normal.z;
	float Y1_1 = 0.488603 * thefrag_Normal.y;
	float Y21 = 1.092548 * thefrag_Normal.x*thefrag_Normal.z;
	float Y2_1 = 1.092548 * thefrag_Normal.y*thefrag_Normal.z;
	float Y2_2 = 1.092548 * thefrag_Normal.y*thefrag_Normal.x;
	float Y20 = 0.946176 * thefrag_Normal.z * thefrag_Normal.z - 0.315392;
	float Y22 = 0.546274 * (thefrag_Normal.x*thefrag_Normal.x - thefrag_Normal.y*thefrag_Normal.y);

	vec4 color = A0 * Y00 * uni_SH9[uni_SH9Index].L00
		+ A1 * Y1_1 * uni_SH9[uni_SH9Index].L1_1 + A1 * Y10 * uni_SH9[uni_SH9Index].L10 + A1 * Y11 * uni_SH9[uni_SH9Index].L11
		+ A2 * Y2_2 * uni_SH9[uni_SH9Index].L2_2 + A2 * Y2_1 * uni_SH9[uni_SH9Index].L2_1 + A2 * Y20 * uni_SH9[uni_SH9Index].L20 + A2 * Y21 * uni_SH9[uni_SH9Index].L21 + A2 * Y22 * uni_SH9[uni_SH9Index].L22;

	uni_SHVisualization = vec4(color.rgb, 1.0f);
}