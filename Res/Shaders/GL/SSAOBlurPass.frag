// shadertype=glsl
#include "common/common.glsl"
layout(location = 0) out vec4 uni_SSAOBlurPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_SSAOPassRT0;

void main()
{
	vec2 texelSize = 1.0 / skyUBO.viewportSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec3 average = vec3(0.0);

	vec2 neighborTexCoordsUp = screenTexCoords + vec2(0.0f, texelSize.y);
	vec2 neighborTexCoordsDown = screenTexCoords + vec2(0.0f, -texelSize.y);
	vec2 neighborTexCoordsLeft = screenTexCoords + vec2(texelSize.x, 0.0f);
	vec2 neighborTexCoordsRight = screenTexCoords + vec2(-texelSize.x, 0.0f);

	vec4 neighborColorUp = texture(uni_SSAOPassRT0, neighborTexCoordsUp);
	average += neighborColorUp.rgb;

	vec4 neighborColorDown = texture(uni_SSAOPassRT0, neighborTexCoordsDown);
	average += neighborColorDown.rgb;

	vec4 neighborColorLeft = texture(uni_SSAOPassRT0, neighborTexCoordsLeft);
	average += neighborColorLeft.rgb;

	vec4 neighborColorRight = texture(uni_SSAOPassRT0, neighborTexCoordsRight);
	average += neighborColorRight.rgb;

	average /= 4.0f;

	uni_SSAOBlurPassRT0 = vec4(average, 1.0);
}