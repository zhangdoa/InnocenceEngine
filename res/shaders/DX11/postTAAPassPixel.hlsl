// shadertype=hlsl

Texture2D in_TAAPassRT0 : register(t0);

SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 postTAAPassRT0 : SV_Target0;
};

float luma(float3 color)
{
	return dot(color, float3(0.299, 0.587, 0.114));
}

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float3 finalColor = in_TAAPassRT0.Sample(SampleTypePoint, input.texcoord).rgb;

	// Undo tone mapping
	finalColor = finalColor / (1.0f - luma(finalColor));

	output.postTAAPassRT0 = float4(finalColor, 1.0);

	return output;
}