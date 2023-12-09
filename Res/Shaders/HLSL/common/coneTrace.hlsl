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

struct ConeTraceResult
{
	float4 color;
	float4 info;
};

inline ConeTraceResult ConeTrace(
	in Texture3D<float4> voxelTexture,
	in SamplerState samplerTypePoint,
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

		float4 sam = voxelTexture.SampleLevel(samplerTypePoint, tc, mip);

		color += sam.rgb;
		if (sam.a == 1.0)
			break;

		dist += diameter * voxelizationPassCBuffer.coneTracingStep;
	}

	ConeTraceResult l_result;

	l_result.color = float4(color, 1.0f);
	l_result.info = float4(dist, 0.0f, 0.0f, 0.0f);

	return l_result;
}

inline float4 ConeTraceRadianceDiffuse(
	in Texture3D<float4> voxelTexture,
	in SamplerState samplerTypePoint,
	in float3 P,
	in float3 N,
	in VoxelizationPass_CB voxelizationPassCBuffer
)
{
	float4 radiance = 0;

	for (uint cone = 0; cone < 16; ++cone)
	{
		float3 coneDirection = normalize(CONES[cone] + N);

		coneDirection *= dot(coneDirection, N) < 0 ? -1 : 1;

		radiance += ConeTrace(voxelTexture, samplerTypePoint, P, N, coneDirection, tan(PI * 0.5f * 0.33f), voxelizationPassCBuffer).color;
	}

	radiance /= 16;

	return max(0, radiance);
}

inline float4 ConeTraceRadianceSpecular(
	in Texture3D<float4> voxelTexture,
	in SamplerState samplerTypePoint,
	in float3 P,
	in float3 N,
	in float3 R,
	in VoxelizationPass_CB voxelizationPassCBuffer
)
{
	float3 coneDirection = normalize(R);

	ConeTraceResult l_result = ConeTrace(voxelTexture, samplerTypePoint, P, N, coneDirection, 0, voxelizationPassCBuffer);

	return l_result.color / (l_result.info.x * l_result.info.x);
}