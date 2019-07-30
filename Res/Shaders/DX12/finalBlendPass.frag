// shadertype=hlsl
#include "common.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D basePassRT0 : register(t0);
SamplerState SamplerTypePoint: register(s0);

//Academy Color Encoding System
//http://www.oscars.org/science-technology/sci-tech-projects/aces
float3 acesFilm(const float3 x) {
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(PixelInputType input) : SV_TARGET
{
	float3 finalColor = basePassRT0.Sample(SamplerTypePoint, input.texcoord).xyz;

	//HDR to LDR
	finalColor = acesFilm(finalColor);

	//gamma correction
	float gammaRatio = 1.0 / 2.2;
	finalColor = pow(finalColor, float3(gammaRatio, gammaRatio, gammaRatio));

	return float4(finalColor, 1.0f);
}