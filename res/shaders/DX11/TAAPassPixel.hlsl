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

	float2 historyTexCoords = screenTexCoords - MotionVector;
	float3 historyColor = in_history.Sample(SampleTypePoint, historyTexCoords).rgb;

	float3 finalColor = float3(0.0, 0.0, 0.0);

	float3 maxNeighbor = float3(0.0, 0.0, 0.0);
	float3 minNeighbor = float3(1.0, 1.0, 1.0);
	float3 averageNeighbor = float3(0.0, 0.0, 0.0);

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
	averageNeighbor = neighborSum / 9.0;

	historyColor = clamp(historyColor, minNeighbor, maxNeighbor);
	float subpixelCorrection = frac(max(abs(MotionVector.x)*renderTargetSize.x, abs(MotionVector.y)*renderTargetSize.y));
	float contrast = distance(averageNeighbor, currentColor);
	float weight = clamp(lerp(1.0, contrast, subpixelCorrection) * 0.05, 0.0, 1.0);
	finalColor = lerp(historyColor, currentColor, weight);

	output.TAAPassRT0 = float4(finalColor, 1.0);

	return output;
}