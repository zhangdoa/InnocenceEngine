// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_bloomExtractPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_TAAPassRT0;

void main()
{
	vec2 texelSize = 1.0 / perFrameCBuffer.data.viewportSize.xy;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec4 currentColor = texture(uni_TAAPassRT0, screenTexCoords);
	if (currentColor.a == 0.0)
	{
		discard;
	}

	vec3 TAAPassRT0ExtractedResult = texture(uni_TAAPassRT0, screenTexCoords).rgb;
	float brightness = 0.0;
	brightness = dot(TAAPassRT0ExtractedResult.rgb, vec3(0.2126, 0.7152, 0.0722));
	if (brightness > 1.0)
	{
		uni_bloomExtractPassRT0 = vec4(TAAPassRT0ExtractedResult, 1.0);
	}
	else
	{
		uni_bloomExtractPassRT0 = vec4(0.0);
	}
}