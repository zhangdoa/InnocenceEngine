// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(location = 0, binding = 0) uniform sampler2D uni_basePassRT0;

void main()
{
	vec2 texelSize = 1.0 / perFrameCBuffer.data.viewportSize.xy;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec3 finalColor;
	vec4 basePassResult = texture(uni_basePassRT0, screenTexCoords);
	finalColor = basePassResult.rgb;
	FragColor = vec4(finalColor, 1.0);
}