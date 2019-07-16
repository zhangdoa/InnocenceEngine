// shadertype=glsl
#include "common.glsl"
layout(location = 0) out vec4 uni_TAAPassRT0;

layout(location = 0) in vec2 TexCoords;

layout(location = 0, binding = 0) uniform sampler2D uni_preTAAPassRT0;
layout(location = 1, binding = 1) uniform sampler2D uni_history;
layout(location = 2, binding = 2) uniform sampler2D uni_motionVectorTexture;

#define Use_YCoCg 0

// [https://software.intel.com/en-us/node/503873]
vec3 RGB_YCoCg(vec3 c)
{
	// Y = R/4 + G/2 + B/4
	// Co = R/2 - B/2
	// Cg = -R/4 + G/2 - B/4
	return vec3(
		c.x / 4.0 + c.y / 2.0 + c.z / 4.0,
		c.x / 2.0 - c.z / 2.0,
		-c.x / 4.0 + c.y / 2.0 - c.z / 4.0
	);
}

// [https://software.intel.com/en-us/node/503873]
vec3 YCoCg_RGB(vec3 c)
{
	// R = Y + Co - Cg
	// G = Y + Cg
	// B = Y - Co - Cg
	return clamp(vec3(
		c.x + c.y - c.z,
		c.x + c.z,
		c.x - c.y - c.z
	), vec3(0.0), vec3(1.0));
}

float luma(vec3 color)
{
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
	vec2 renderTargetSize = vec2(textureSize(uni_preTAAPassRT0, 0));
	vec2 texelSize = 1.0 / renderTargetSize;
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;
	vec2 MotionVector = texture(uni_motionVectorTexture, screenTexCoords).xy;
	vec3 currentColor = texture(uni_preTAAPassRT0, screenTexCoords).rgb;

	// Tone mapping
	float lumaCurrentColor = luma(currentColor);
	currentColor = currentColor / (1.0f + lumaCurrentColor);

	vec2 historyTexCoords = screenTexCoords - MotionVector;
	vec3 historyColor = texture(uni_history, historyTexCoords).rgb;
	float lumaHistoryColor = luma(historyColor);

	vec3 maxNeighbor = vec3(0.0);
	vec3 minNeighbor = vec3(1.0);
	vec3 neighborSum = vec3(0.0);

	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			vec2 neighborTexCoords = screenTexCoords + vec2(float(x) / renderTargetSize.x, float(y) / renderTargetSize.y);
			vec3 neighborColor = texture(uni_preTAAPassRT0, neighborTexCoords).rgb;
			maxNeighbor = max(maxNeighbor, neighborColor);
			minNeighbor = min(minNeighbor, neighborColor);
			neighborSum += neighborColor;
		}
	}

	vec3 neighborAverage = neighborSum / 9.0;

#if Use_YCoCg
	// Clamp history color's chroma in YCoCg space
	currentColor = RGB_YCoCg(currentColor);
	historyColor = RGB_YCoCg(historyColor);
	maxNeighbor = RGB_YCoCg(maxNeighbor);
	minNeighbor = RGB_YCoCg(minNeighbor);

	float chroma_extent_element = 0.25 * 0.5 * (maxNeighbor.x - minNeighbor.x);
	vec2 chroma_extent = vec2(chroma_extent_element, chroma_extent_element);
	vec2 chroma_center = currentColor.yz;
	minNeighbor.yz = chroma_center - chroma_extent;
	maxNeighbor.yz = chroma_center + chroma_extent;
	neighborAverage.yz = chroma_center;
#endif

	historyColor = clamp(historyColor, minNeighbor, maxNeighbor);

	// Mix by dynamic weight
	float unbiased_diff = abs(lumaCurrentColor - lumaHistoryColor) / max(lumaCurrentColor, max(lumaHistoryColor, 0.2));
	float unbiased_weight = 1.0 - unbiased_diff;
	float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
	float weight = mix(0.01, 0.05, unbiased_weight_sqr);
	vec3 finalColor = mix(historyColor, currentColor, weight);

#if Use_YCoCg
	// Return to RGB space
	finalColor = YCoCg_RGB(finalColor);
#endif

	uni_TAAPassRT0 = vec4(finalColor, lumaCurrentColor);
}