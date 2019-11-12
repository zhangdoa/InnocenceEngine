// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_motionBlurPassRT0;
layout(location = 0) in vec2 thefrag_TexCoord;

layout(location = 0, set = 1, binding = 0) uniform texture2D uni_motionVectorTexture;
layout(location = 1, set = 1, binding = 1) uniform texture2D uni_TAAPassRT0;

layout(set = 2, binding = 0) uniform sampler samplerLinear;

const int MAX_SAMPLES = 16;

void main()
{
	vec2 texelSize = 1.0 / skyUBO.viewportSize.xy;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec2 MotionVector = texture(sampler2D(uni_motionVectorTexture, samplerLinear), screenTexCoords).xy;

	vec4 result = texture(sampler2D(uni_TAAPassRT0, samplerLinear), screenTexCoords);

	float half_samples = float(MAX_SAMPLES / 2);

	// sample half samples along motion vector and another half in opposite direction
	for (int i = 1; i <= half_samples; i++) {
		vec2 offset = MotionVector * (float(i) / float(MAX_SAMPLES));
		result += texture(sampler2D(uni_TAAPassRT0, samplerLinear), screenTexCoords - offset);
		result += texture(sampler2D(uni_TAAPassRT0, samplerLinear), screenTexCoords + offset);
	}

	result /= float(MAX_SAMPLES + 1);

	//use alpha channel as mask
	uni_motionBlurPassRT0 = vec4(result.rgb, 1.0);
}