// shadertype=hlsl
#include "common/common.hlsl"

#define Use_YCoCg 0
Texture2D in_preTAAPassRT0 : register(t0);
Texture2D in_history : register(t1);
Texture2D in_opaquePassRT3 : register(t2);

SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 TAAPassRT0 : SV_Target0;
};

// [https://software.intel.com/en-us/node/503873]
float3 RGB_YCoCg(float3 c)
{
	// Y = R/4 + G/2 + B/4
	// Co = R/2 - B/2
	// Cg = -R/4 + G/2 - B/4
	return float3(
		c.x / 4.0 + c.y / 2.0 + c.z / 4.0,
		c.x / 2.0 - c.z / 2.0,
		-c.x / 4.0 + c.y / 2.0 - c.z / 4.0
	);
}

// [https://software.intel.com/en-us/node/503873]
float3 YCoCg_RGB(float3 c)
{
	// R = Y + Co - Cg
	// G = Y + Cg
	// B = Y - Co - Cg
	return float3(
		c.x + c.y - c.z,
		c.x + c.z,
		c.x - c.y - c.z
	);
}

float3 tonemap(float3 color)
{
	float maxValue = max(max(color.x, color.y), color.z);
	return color / (1.0f + maxValue);
}

float3 tonemapInvert(float3 color)
{
	float maxValue = max(max(color.x, color.y), color.z);
	return color / (1.0f - maxValue);
}

float luma(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float2 renderTargetSize;
	float level;
	in_preTAAPassRT0.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, level);
	float2 texelSize = 1.0 / renderTargetSize;
	float2 screenTexCoords = input.position.xy * texelSize;
	float3 currentColor = in_preTAAPassRT0.Sample(SampleTypePoint, screenTexCoords).rgb;

	float2 MotionVector = in_opaquePassRT3.Sample(SampleTypePoint, screenTexCoords).xy;
	float2 historyTexCoords = screenTexCoords - MotionVector;
	historyTexCoords = saturate(historyTexCoords);

	float3 historyColor = in_history.Sample(SampleTypePoint, historyTexCoords).rgb;

	float3 maxNeighbor = float3(FLT_MIN, FLT_MIN, FLT_MIN);
	float3 minNeighbor = float3(FLT_MAX, FLT_MAX, FLT_MAX);

	float3 neighborSum = float3(0.0, 0.0, 0.0);

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			float2 neighborTexCoords = screenTexCoords + float2(float(x) / renderTargetSize.x, float(y) / renderTargetSize.y);

			neighborTexCoords = saturate(neighborTexCoords);

			float3 neighborColor = in_preTAAPassRT0.Sample(SampleTypePoint, neighborTexCoords).rgb;

			maxNeighbor = max(maxNeighbor, neighborColor);
			minNeighbor = min(minNeighbor, neighborColor);
			neighborSum += neighborColor.rgb;
		}
	}

	float3 neighborAverage = neighborSum / 9.0;

#if Use_YCoCg
	// Clamp history color's chroma in YCoCg space
	currentColor = RGB_YCoCg(currentColor);
	historyColor = RGB_YCoCg(historyColor);
	maxNeighbor = RGB_YCoCg(maxNeighbor);
	minNeighbor = RGB_YCoCg(minNeighbor);
	neighborAverage = RGB_YCoCg(neighborAverage);

	float chroma_extent_element = 0.25 * 0.5 * (maxNeighbor.x - minNeighbor.x);
	float2 chroma_extent = float2(chroma_extent_element, chroma_extent_element);
	float2 chroma_center = currentColor.yz;
	minNeighbor.yz = chroma_center - chroma_extent;
	maxNeighbor.yz = chroma_center + chroma_extent;
	neighborAverage.yz = chroma_center;

	float lumaCurrentColor = currentColor.x;
	float lumaHistoryColor = historyColor.x;
#else
	float lumaCurrentColor = luma(currentColor);
	float lumaHistoryColor = luma(historyColor);
#endif

	historyColor = clamp(historyColor, minNeighbor, maxNeighbor);

	// Mix by dynamic weight
	float unbiased_diff = abs(lumaCurrentColor - lumaHistoryColor) / max(lumaCurrentColor, max(lumaHistoryColor, 0.2));
	float unbiased_weight = 1.0 - unbiased_diff;
	float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
	float weight = lerp(0.01, 0.05, unbiased_weight_sqr);
	float3 finalColor = lerp(historyColor, currentColor, weight);

#if Use_YCoCg
	// Return to RGB space
	finalColor = YCoCg_RGB(finalColor);
#endif

	output.TAAPassRT0 = float4(finalColor, lumaCurrentColor);

	return output;
}