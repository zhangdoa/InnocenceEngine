// shadertype=<glsl>
#version 450
layout(location = 0) out vec4 uni_SSAOBlurPassRT0;

in vec2 TexCoords;

uniform sampler2D uni_SSAOPassRT0;

void main()
{	
	vec2 renderTargetSize = vec2(textureSize(uni_SSAOPassRT0, 0));

	vec3 average = vec3(0.0);

	vec3 neighborColorSum = vec3(0.0);
	float validNeighborNum = 0.0;

	vec3 finalColor = vec3(0.0);

	vec2 neighborTexCoordsUp = TexCoords + vec2(0.0f, 1.0f / renderTargetSize.y);
	vec2 neighborTexCoordsDown = TexCoords + vec2(0.0f, -1.0f / renderTargetSize.y);
	vec2 neighborTexCoordsLeft = TexCoords + vec2(1.0f / renderTargetSize.x, 0.0f);
	vec2 neighborTexCoordsRight = TexCoords + vec2(-1.0f / renderTargetSize.x, 0.0f);

	vec4 neighborColorUp = texture(uni_SSAOPassRT0, neighborTexCoordsUp);
	if (neighborColorUp.a != 0.0)
	{
		average += neighborColorUp.rgb;
		validNeighborNum += 1.0;
	}
	vec4 neighborColorDown = texture(uni_SSAOPassRT0, neighborTexCoordsDown);
	if (neighborColorDown.a != 0.0)
	{
		average += neighborColorDown.rgb;
		validNeighborNum += 1.0;
	}
	vec4 neighborColorLeft = texture(uni_SSAOPassRT0, neighborTexCoordsLeft);
	if (neighborColorLeft.a != 0.0)
	{
		average += neighborColorLeft.rgb;
		validNeighborNum += 1.0;
	}
	vec4 neighborColorRight = texture(uni_SSAOPassRT0, neighborTexCoordsRight);
	if (neighborColorRight.a != 0.0)
	{
		average += neighborColorRight.rgb;
		validNeighborNum += 1.0;
	}

	average /= validNeighborNum;

	uni_SSAOBlurPassRT0 = vec4(average, 1.0);
}
