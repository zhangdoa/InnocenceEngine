// shadertype=hlsl
#include "common/common.hlsl"

Texture3D<float4> probeVolume : register(t1);

SamplerState SamplerTypeLinear : register(s0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION_WS;
	float4 probeIndex : PROBE_INDEX;
	float4 normal : NORMAL;
};

struct PixelOutputType
{
	float4 probeTestPassRT0 : SV_Target0;
};

PixelOutputType main(PixelInputType input) : SV_TARGET
{
	PixelOutputType output;

	float3 N = normalize(input.normal.xyz);
	float3 nSquared = N * N;
	int3 isNegative = (N < 0.0);
	float3 GISampleCoord = (input.posWS.xyz - GICBuffer.irradianceVolumeOffset.xyz) / perFrameCBuffer.posWSNormalizer.xyz;

	float3 indirectLight = float3(0.0f, 0.0f, 0.0f);

	GISampleCoord.z /= 6.0;
	float3 GISampleCoordPX = GISampleCoord;
	float3 GISampleCoordNX = GISampleCoord + float3(0, 0, 1.0 / 6.0);
	float3 GISampleCoordPY = GISampleCoord + float3(0, 0, 2.0 / 6.0);
	float3 GISampleCoordNY = GISampleCoord + float3(0, 0, 3.0 / 6.0);
	float3 GISampleCoordPZ = GISampleCoord + float3(0, 0, 4.0 / 6.0);
	float3 GISampleCoordNZ = GISampleCoord + float3(0, 0, 5.0 / 6.0);

	if (isNegative.x)
	{
		indirectLight += nSquared.x * probeVolume.Sample(SamplerTypeLinear, GISampleCoordNX);
	}
	else
	{
		indirectLight += nSquared.x * probeVolume.Sample(SamplerTypeLinear, GISampleCoordPX);
	}
	if (isNegative.y)
	{
		indirectLight += nSquared.y * probeVolume.Sample(SamplerTypeLinear, GISampleCoordNY);
	}
	else
	{
		indirectLight += nSquared.y * probeVolume.Sample(SamplerTypeLinear, GISampleCoordPY);
	}
	if (isNegative.z)
	{
		indirectLight += nSquared.z * probeVolume.Sample(SamplerTypeLinear, GISampleCoordNZ);
	}
	else
	{
		indirectLight += nSquared.z * probeVolume.Sample(SamplerTypeLinear, GISampleCoordPZ);
	}
	output.probeTestPassRT0 = float4(indirectLight, 1.0);

	return output;
}