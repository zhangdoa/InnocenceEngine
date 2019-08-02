// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec2 uni_brdfMSLUT;
layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_brdfLUT;

// ----------------------------------------------------------------------------
void main()
{
	float averangeRsF1 = 0.0;
	float currentRsF1 = 0.0;
	const uint textureSize = 512u;
	// "Real-Time Rendering", 4th edition, pg. 346, "9.8.2 Multiple-Bounce Surface Reflection", "The function $\overline{RsF1}$ is the cosine-weighted average value of RsF1 over the hemisphere"
	for (uint i = 0u; i < textureSize; ++i)
	{
		currentRsF1 = texture(uni_brdfLUT, vec2(float(i) / float(textureSize), TexCoords.y)).b;
		currentRsF1 /= float(textureSize);
		averangeRsF1 += currentRsF1;
	}

	uni_brdfMSLUT = vec2(averangeRsF1, 0.0);
}