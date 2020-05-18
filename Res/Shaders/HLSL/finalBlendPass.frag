// shadertype=hlsl
#include "common/common.hlsl"
//#define AUTO_EXPOSURE

Texture2D basePassRT0 : register(t0);
Texture2D billboardPassRT0 : register(t1);
Texture2D debugPassRT0 : register(t2);
StructuredBuffer<float> in_luminanceAverage : register(t3);

SamplerState SamplerTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float3 basePass = basePassRT0.Sample(SamplerTypePoint, input.texcoord).xyz;
	float4 billboardPass = billboardPassRT0.Sample(SamplerTypePoint, input.texcoord);
	float4 debugPass = debugPassRT0.Sample(SamplerTypePoint, input.texcoord);

	// HDR to LDR
#ifdef AUTO_EXPOSURE
	float EV100 = ComputeEV100FromAvgLuminance(in_luminanceAverage[0]);
#else
	float EV100 = ComputeEV100(perFrameCBuffer.aperture, perFrameCBuffer.shutterTime, perFrameCBuffer.ISO);
#endif
	float exposure = ConvertEV100ToExposure(EV100);

	float3 bassPassxyY = RGB_XYY(basePass);
	bassPassxyY.z *= exposure;

	basePass = XYY_RGB(bassPassxyY);

	// Tone Mapping
	float3 finalColor = TonemapACES(basePass);

	// Gamma Correction
	finalColor = AccurateLinearToSRGB(finalColor);

	// billboard overlay
	finalColor += billboardPass.rgb;

	// debug overlay
	if (debugPass.a == 1.0)
	{
		finalColor = debugPass.rgb;
	}

	return float4(finalColor, 1.0f);
}