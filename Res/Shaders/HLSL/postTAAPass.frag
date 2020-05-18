// shadertype=hlsl
#include "common/common.hlsl"

Texture2D in_TAAPassRT0 : register(t0);

SamplerState SamplerTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 postTAAPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float2 renderTargetSize;
	float level;
	in_TAAPassRT0.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, level);
	float2 texelSize = 1.0 / renderTargetSize;
	float2 screenTexCoords = input.position.xy * texelSize;
	float4 TAAResult = in_TAAPassRT0.Sample(SamplerTypePoint, screenTexCoords);
	float3 currentColor = TAAResult.rgb;

	// Undo tone mapping
	float3 finalColor = TonemapInvertReinhardLuma(currentColor);

	output.postTAAPassRT0 = float4(currentColor, 1.0);

	return output;
}