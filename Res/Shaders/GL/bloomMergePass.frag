// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_bloomMergePassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_Full;

void main()
{
	vec2 texelSize = 1.0 / skyUBO.viewportSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec3 finalColor = texture(uni_Full, screenTexCoords).rgb;
	uni_bloomMergePassRT0 = vec4(finalColor, 1.0f);
}