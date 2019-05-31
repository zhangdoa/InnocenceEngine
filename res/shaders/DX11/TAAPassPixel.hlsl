// shadertype=hlsl

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
	return clamp(float3(
		c.x + c.y - c.z,
		c.x + c.z,
		c.x - c.y - c.z
		), float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
}

float luma(float3 color)
{
	return dot(color, float3(0.299, 0.587, 0.114));
}

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float2 renderTargetSize;
	float level;
	in_preTAAPassRT0.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, level);
	float2 texelSize = 1.0 / renderTargetSize;
	float2 screenTexCoords = input.position.xy * texelSize;
	float2 MotionVector = in_opaquePassRT3.Sample(SampleTypePoint, screenTexCoords).xy;
	float3 currentColor = in_preTAAPassRT0.Sample(SampleTypePoint, screenTexCoords).rgb;

	// Tone mapping
	currentColor = currentColor / (1.0f + luma(currentColor));

	float2 historyTexCoords = screenTexCoords - MotionVector;
	float3 historyColor = in_history.Sample(SampleTypePoint, historyTexCoords).rgb;

	float3 maxNeighbor = float3(0.0, 0.0, 0.0);
	float3 minNeighbor = float3(1.0, 1.0, 1.0);

	float3 neighborSum = float3(0.0, 0.0, 0.0);

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			float2 neighborTexCoords = screenTexCoords + float2(float(x) / renderTargetSize.x, float(y) / renderTargetSize.y);
			float3 neighborColor = in_preTAAPassRT0.Sample(SampleTypePoint, neighborTexCoords).rgb;
			maxNeighbor = max(maxNeighbor, neighborColor);
			minNeighbor = min(minNeighbor, neighborColor);
			neighborSum += neighborColor.rgb;
		}
	}

	float3 neighborAverage = neighborSum / 9.0;

	// Mix in YCoCg space
	currentColor = RGB_YCoCg(currentColor);
	historyColor = RGB_YCoCg(historyColor);
	maxNeighbor = RGB_YCoCg(maxNeighbor);
	minNeighbor = RGB_YCoCg(minNeighbor);
	neighborAverage = RGB_YCoCg(neighborAverage);

	float chroma_extent_element = 0.25 * 0.5 * (maxNeighbor.r - minNeighbor.r);
	float2 chroma_extent = float2(chroma_extent_element, chroma_extent_element);
	float2 chroma_center = currentColor.gb;
	minNeighbor.yz = chroma_center - chroma_extent;
	maxNeighbor.yz = chroma_center + chroma_extent;
	neighborAverage.yz = chroma_center;

	historyColor = clamp(historyColor, minNeighbor, maxNeighbor);

	float subpixelCorrection = frac(max(abs(MotionVector.x)*renderTargetSize.x, abs(MotionVector.y)*renderTargetSize.y));
	float contrast = distance(neighborAverage, currentColor);
	float weight = clamp(lerp(1.0, contrast, subpixelCorrection) * 0.05, 0.0, 1.0);
	float3 finalColor = lerp(historyColor, currentColor, weight);

	// Return to RGB space
	finalColor = YCoCg_RGB(finalColor);

	output.TAAPassRT0 = float4(finalColor, 1.0);

	return output;
}