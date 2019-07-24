// shadertype=glsl
#include "common.glsl"
layout(location = 0) out vec4 uni_postTAAPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_lastTAAPassRT0;

float luma(vec3 color) {
	return dot(color, vec3(0.299, 0.587, 0.114));
}

void main()
{
	// sharpen the TAA result [Siggraph 2016 "Temporal Antialiasing in Uncharted 4"]
	vec2 texelSize = 1.0 / uni_viewportSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec4 TAAResult = texture(uni_lastTAAPassRT0, screenTexCoords);
	vec3 currentColor = TAAResult.rgb;
	float luma = TAAResult.a;

	vec3 average = vec3(0.0);
	vec3 neighborColorSum = vec3(0.0);
	vec3 finalColor = vec3(0.0);

	vec2 TexCoordsUp = screenTexCoords + vec2(0.0f, texelSize.y);
	vec2 TexCoordsDown = screenTexCoords + vec2(0.0f, -texelSize.y);
	vec2 TexCoordsLeft = screenTexCoords + vec2(texelSize.x, 0.0f);
	vec2 TexCoordsRight = screenTexCoords + vec2(-texelSize.x, 0.0f);

	vec4 ColorUp = texture(uni_lastTAAPassRT0, TexCoordsUp);
	average -= ColorUp.rgb;

	vec4 ColorDown = texture(uni_lastTAAPassRT0, TexCoordsDown);
	average -= ColorDown.rgb;

	vec4 ColorLeft = texture(uni_lastTAAPassRT0, TexCoordsLeft);
	average -= ColorLeft.rgb;

	vec4 ColorRight = texture(uni_lastTAAPassRT0, TexCoordsRight);
	average -= ColorRight.rgb;

	finalColor = currentColor + average + currentColor * 4.0;

	finalColor = currentColor;

	// Undo tone mapping
	finalColor = finalColor * (1.0f + luma);

	uni_postTAAPassRT0 = vec4(finalColor, 1.0);
}