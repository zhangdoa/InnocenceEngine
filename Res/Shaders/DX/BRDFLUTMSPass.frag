// shadertype=hlsl
#include "common/common.hlsl"

Texture2D in_brdfLUT : register(t0);

SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

struct PixelOutputType
{
	float4 brdfMSLUT : SV_Target0;
};
// ----------------------------------------------------------------------------
PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float2 renderTargetSize;
	float level;
	in_brdfLUT.GetDimensions(0, renderTargetSize.x, renderTargetSize.y, level);

	float averangeRsF1 = 0.0;
	float currentRsF1 = 0.0;
	const uint textureSize = 512u;
	// "Real-Time Rendering", 4th edition, pg. 346, "9.8.2 Multiple-Bounce Surface Reflection", "The function $\overline{RsF1}$ is the cosine-weighted average value of RsF1 over the hemisphere"
	for (uint i = 0u; i < textureSize; ++i)
	{
		float cos = float(i) / float(renderTargetSize.x);
		currentRsF1 = in_brdfLUT.Sample(SampleTypePoint, float2(cos, input.texcoord.y)).b;
		currentRsF1 /= float(textureSize);
		// cos-weighted
		currentRsF1 *= cos;
		averangeRsF1 += currentRsF1;
	}

	output.brdfMSLUT = float4(averangeRsF1, averangeRsF1, averangeRsF1, 1.0);
	return output;
}