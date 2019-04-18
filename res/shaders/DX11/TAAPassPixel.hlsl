// shadertype=hlsl

Texture2D in_preTAAPassRT0 : register(t0);
Texture2D in_history3 : register(t1);
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
	float2 MotionVector = in_opaquePassRT3.Sample(SampleTypePoint, input.texcoord).xy;

	float4 preTAAPassRT0 = in_preTAAPassRT0.Sample(SampleTypePoint, input.texcoord);

	float3 currentColor = preTAAPassRT0.rgb;

	float2 historyTexCoords = input.texcoord - MotionVector;
	float4 historyColor;

	float4 historyColor3 = in_history3.Sample(SampleTypePoint, historyTexCoords);

	historyColor = historyColor3;

	float3 finalColor = float3(0.0, 0.0, 0.0);

	float3 maxNeighbor = float3(0.0, 0.0, 0.0);
	float3 minNeighbor = float3(1.0, 1.0, 1.0);
	float4 neighborColor = float4(0.0, 0.0, 0.0, 0.0);
	float3 average = float3(0.0, 0.0, 0.0);

	float3 neighborColorSum = float3(0.0, 0.0, 0.0);
	float validNeighborNum = 0.0;

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			float2 neighborTexCoords = input.texcoord + float2(float(x) / renderTargetSize.x, float(y) / renderTargetSize.y);
			neighborColor = in_preTAAPassRT0.Sample(SampleTypePoint, neighborTexCoords);
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
	float subpixelCorrection = frac(max(abs(MotionVector.x)*renderTargetSize.x, abs(MotionVector.y)*renderTargetSize.y));
	float contrast = distance(average, currentColor.rgb);
	float weight = clamp(lerp(1.0, contrast, subpixelCorrection) * 0.05, 0.0, 1.0);
	finalColor = lerp(historyColor.rgb, currentColor.rgb, weight);

	output.TAAPassRT0 = float4(finalColor, 1.0);

	return output;
}