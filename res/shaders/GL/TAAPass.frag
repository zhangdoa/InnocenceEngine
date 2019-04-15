// shadertype=<glsl>
#version 450
layout(location = 0) out vec4 uni_TAAPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_preTAAPassRT0;
layout(location = 1, binding = 1) uniform sampler2D uni_history0;
layout(location = 2, binding = 2) uniform sampler2D uni_history1;
layout(location = 3, binding = 3) uniform sampler2D uni_history2;
layout(location = 4, binding = 4) uniform sampler2D uni_history3;
layout(location = 5, binding = 5) uniform sampler2D uni_motionVectorTexture;

void main()
{
	vec2 renderTargetSize = vec2(textureSize(uni_preTAAPassRT0, 0));
	vec2 texelSize = 1.0 / renderTargetSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;
	vec2 MotionVector = texture(uni_motionVectorTexture, screenTexCoords).xy;
	MotionVector.y = -MotionVector.y;

	vec4 preTAAPassRT0Result = texture(uni_preTAAPassRT0, TexCoords);

	vec3 currentColor = preTAAPassRT0Result.rgb;

	vec2 historyTexCoords = screenTexCoords - MotionVector;
	vec4 historyColor;

	vec4 historyColor0 = texture(uni_history0, historyTexCoords);
	vec4 historyColor1 = texture(uni_history1, historyTexCoords);
	vec4 historyColor2 = texture(uni_history2, historyTexCoords);

	vec4 historyColor3 = texture(uni_history3, historyTexCoords);

	historyColor = historyColor0 + historyColor1 + historyColor2 + historyColor3;
	historyColor /= 4.0f;

	vec3 maxNeighbor = vec3(0.0);
	vec3 minNeighbor = vec3(1.0);
	vec4 neighborColor = vec4(0.0);
	vec3 average = vec3(0.0);

	vec3 neighborColorSum = vec3(0.0);
	float validNeighborNum = 0.0;

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			vec2 neighborTexCoords = screenTexCoords + vec2(float(x) / renderTargetSize.x, float(y) / renderTargetSize.y);
			neighborColor = texture(uni_preTAAPassRT0, neighborTexCoords);
			if (neighborColor.a != 0.0)
			{
				maxNeighbor = max(maxNeighbor, neighborColor.rgb);
				minNeighbor = min(minNeighbor, neighborColor.rgb);
				neighborColorSum += neighborColor.rgb;
				validNeighborNum += 1.0;
			}
		}
	}
	average = neighborColorSum / validNeighborNum;

	historyColor.rgb = clamp(historyColor.rgb, minNeighbor, maxNeighbor);
	float subpixelCorrection = fract(max(abs(MotionVector.x)*renderTargetSize.x, abs(MotionVector.y)*renderTargetSize.y));
	float contrast = distance(average, currentColor.rgb);
	float weight = clamp(mix(1.0, contrast, subpixelCorrection) * 0.05, 0.0, 1.0);
	vec3 finalColor = mix(historyColor.rgb, currentColor.rgb, weight);

	uni_TAAPassRT0 = vec4(finalColor, 1.0);
}