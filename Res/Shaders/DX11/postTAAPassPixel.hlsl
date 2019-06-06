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

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float3 finalColor = in_TAAPassRT0.Sample(SampleTypePoint, input.texcoord).rgb;
	float lumaCurrentColor = in_TAAPassRT0.Sample(SampleTypePoint, input.texcoord).a;

	// Undo tone mapping
	finalColor = finalColor * (1.0f + lumaCurrentColor);

	output.postTAAPassRT0 = float4(finalColor, 1.0);

	return output;
}