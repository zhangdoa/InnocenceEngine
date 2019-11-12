// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_preTAAPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, set = 1, binding = 0) uniform texture2D uni_lightPassRT0;
layout(location = 1, set = 1, binding = 1) uniform texture2D uni_skyPassRT0;

layout(set = 2, binding = 0) uniform sampler samplerLinear;

void main()
{
	vec2 screenTextureSize = textureSize(uni_lightPassRT0, 0);
	vec2 texelSize = 1.0 / screenTextureSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec4 lightPassResult = texture(sampler2D(uni_lightPassRT0, samplerLinear), screenTexCoords);
	vec4 skyPassResult = texture(sampler2D(uni_skyPassRT0, samplerLinear), screenTexCoords);

	vec3 finalColor = vec3(0.0);

	if (lightPassResult.a == 0.0)
	{
		finalColor += skyPassResult.rgb;
	}
	else
	{
		finalColor += lightPassResult.rgb;
	}

	uni_preTAAPassRT0 = vec4(finalColor, 1.0);
}