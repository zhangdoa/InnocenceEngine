// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_motionBlurPassRT0;
layout(location = 0) in vec2 thefrag_TexCoord;

layout(location = 0, binding = 0) uniform sampler2D uni_motionVectorTexture;
layout(location = 1, binding = 1) uniform sampler2D uni_TAAPassRT0;

const int MAX_SAMPLES = 16;

void main()
{
	vec2 texelSize = 1.0 / skyUBO.viewportSize.xy;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec2 MotionVector = texture(uni_motionVectorTexture, screenTexCoords).xy;

	vec4 result = texture(uni_TAAPassRT0, screenTexCoords);

	float half_samples = float(MAX_SAMPLES / 2);

	// sample half samples along motion vector and another half in opposite direction
	for (int i = 1; i <= half_samples; i++) {
		vec2 offset = MotionVector * (float(i) / float(MAX_SAMPLES));
		result += texture(uni_TAAPassRT0, screenTexCoords - offset);
		result += texture(uni_TAAPassRT0, screenTexCoords + offset);
	}

	result /= float(MAX_SAMPLES + 1);

	//use alpha channel as mask
	uni_motionBlurPassRT0 = vec4(result.rgb, 1.0);
}