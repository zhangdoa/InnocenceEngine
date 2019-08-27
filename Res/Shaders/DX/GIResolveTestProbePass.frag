// shadertype=hlsl
#include "common/common.hlsl"

Texture3D<float4> probeVolume : register(t1);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
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

	float3 indirectLight = float3(0.0f, 0.0f, 0.0f);

	if (isNegative.x)
	{
		indirectLight += nSquared.x * probeVolume[input.probeIndex.xyz + float3(0, 0, GISky_probeCount.z)];
	}
	else
	{
		indirectLight += nSquared.x * probeVolume[input.probeIndex.xyz];
	}
	if (isNegative.y)
	{
		indirectLight += nSquared.y * probeVolume[input.probeIndex.xyz + float3(0, 0, GISky_probeCount.z * 3)];
	}
	else
	{
		indirectLight += nSquared.y * probeVolume[input.probeIndex.xyz + float3(0, 0, GISky_probeCount.z * 2)];
	}
	if (isNegative.z)
	{
		indirectLight += nSquared.z * probeVolume[input.probeIndex.xyz + float3(0, 0, GISky_probeCount.z * 5)];
	}
	else
	{
		indirectLight += nSquared.z * probeVolume[input.probeIndex.xyz + float3(0, 0, GISky_probeCount.z * 4)];
	}

	output.probeTestPassRT0 = float4(indirectLight, 1.0);

	return output;
}