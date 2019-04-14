// shadertype=<glsl>
#version 450
layout(location = 0) out vec4 uni_TAASharpenPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_lastTAAPassRT0;

void main()
{
	// sharpen the TAA result [Siggraph 2016 "Temporal Antialiasing in Uncharted 4"]
	vec2 renderTargetSize = vec2(textureSize(uni_lastTAAPassRT0, 0));

	vec4 currentColor = texture(uni_lastTAAPassRT0, TexCoords);
	if (currentColor.a == 0.0)
	{
		discard;
	}

	vec3 average = vec3(0.0);

	vec3 neighborColorSum = vec3(0.0);
	float validNeighborNum = 0.0;

	vec3 finalColor = vec3(0.0);

	vec2 neighborTexCoordsUp = TexCoords + vec2(0.0f, 1.0f / renderTargetSize.y);
	vec2 neighborTexCoordsDown = TexCoords + vec2(0.0f, -1.0f / renderTargetSize.y);
	vec2 neighborTexCoordsLeft = TexCoords + vec2(1.0f / renderTargetSize.x, 0.0f);
	vec2 neighborTexCoordsRight = TexCoords + vec2(-1.0f / renderTargetSize.x, 0.0f);

	average = vec3(0.0);
	validNeighborNum = 0.0;

	vec4 neighborColorUp = texture(uni_lastTAAPassRT0, neighborTexCoordsUp);
	if (neighborColorUp.a != 0.0)
	{
		average -= neighborColorUp.rgb;
		validNeighborNum += 1.0;
	}
	vec4 neighborColorDown = texture(uni_lastTAAPassRT0, neighborTexCoordsDown);
	if (neighborColorDown.a != 0.0)
	{
		average -= neighborColorDown.rgb;
		validNeighborNum += 1.0;
	}
	vec4 neighborColorLeft = texture(uni_lastTAAPassRT0, neighborTexCoordsLeft);
	if (neighborColorLeft.a != 0.0)
	{
		average -= neighborColorLeft.rgb;
		validNeighborNum += 1.0;
	}
	vec4 neighborColorRight = texture(uni_lastTAAPassRT0, neighborTexCoordsRight);
	if (neighborColorRight.a != 0.0)
	{
		average -= neighborColorRight.rgb;
		validNeighborNum += 1.0;
	}

	finalColor = currentColor.rgb + average + currentColor.rgb * validNeighborNum;

	//uni_TAASharpenPassRT0 = vec4(finalColor, 1.0);

	uni_TAASharpenPassRT0 = currentColor;
}