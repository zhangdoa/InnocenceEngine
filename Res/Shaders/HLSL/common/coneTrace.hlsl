#include "common/common.hlsl"

static const float3 CONES[] =
{
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.903007, -0.182696, -0.388844),
	float3(-0.903007, 0.182696, 0.388844),
	float3(0.903007, -0.182696, 0.388844),
	float3(0.903007, 0.182696, -0.388844),
	float3(-0.388844, -0.903007, -0.182696),
	float3(0.388844, -0.903007, 0.182696),
	float3(0.388844, 0.903007, -0.182696),
	float3(-0.388844, 0.903007, 0.182696),
	float3(-0.182696, -0.388844, -0.903007),
	float3(0.182696, 0.388844, -0.903007),
	float3(-0.182696, 0.388844, 0.903007),
	float3(0.182696, -0.388844, 0.903007)
};

inline float4 ConeTrace(
	in Texture3D<float4> voxelTexture,
	in SamplerState SamplerTypePoint,
	in float3 P,
	in float3 N,
	in float3 coneDirection,
	in float coneAperture,
	in VoxelizationPass_CB voxelizationPassCBuffer
)
{
	float3 color = 0;

	float dist = voxelizationPassCBuffer.voxelSize;
	float3 startPos = P + N * voxelizationPassCBuffer.voxelSize * SQRT2;

	while (dist < voxelizationPassCBuffer.coneTracingMaxDistance)
	{
		float diameter = max(voxelizationPassCBuffer.voxelSize, 2 * coneAperture * dist);
		float mip = log2(diameter * voxelizationPassCBuffer.voxelSizeRcp);

		float3 tc = startPos + coneDirection * dist;
		tc = tc - voxelizationPassCBuffer.volumeCenter.xyz;
		tc /= (voxelizationPassCBuffer.volumeExtend * 0.5);
		tc = tc * float3(0.5f, 0.5f, 0.5f) + 0.5f;
		int is_saturated = (tc.x == saturate(tc.x)) && (tc.y == saturate(tc.y)) && (tc.z == saturate(tc.z));

		if (!is_saturated || (mip >= 4))
			break;

		float4 sam = voxelTexture.SampleLevel(SamplerTypePoint, tc, mip);

		color += sam.rgb;
		if (sam.a == 1.0)
			break;

		dist += diameter * voxelizationPassCBuffer.coneTracingStep;
	}

	return float4(color, 1.0f);
}

inline float4 ConeTraceRadiance(
	in Texture3D<float4> voxelTexture,
	in SamplerState SamplerTypePoint,
	in float3 P,
	in float3 N,
	in VoxelizationPass_CB voxelizationPassCBuffer
)
{
	float4 radiance = 0;

	for (uint cone = 0; cone < (uint)voxelizationPassCBuffer.numCones; ++cone)
	{
		float3 coneDirection = normalize(CONES[cone] + N);

		coneDirection *= dot(coneDirection, N) < 0 ? -1 : 1;

		radiance += ConeTrace(voxelTexture, SamplerTypePoint, P, N, coneDirection, tan(PI * 0.5f * 0.33f), voxelizationPassCBuffer);
	}

	radiance *= voxelizationPassCBuffer.numConesRcp;
	radiance.a = saturate(radiance.a);

	return max(0, radiance);
}