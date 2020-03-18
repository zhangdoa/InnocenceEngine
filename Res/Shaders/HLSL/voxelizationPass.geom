// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float4 Normal : NORMAL;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION;
	float4 AABB : AABB;
};

int CalculateAxis(float4 pos[3])
{
	float3 p1 = pos[1].xyz - pos[0].xyz;
	float3 p2 = pos[2].xyz - pos[0].xyz;
	float3 faceNormal = cross(p1, p2);

	float nDX = abs(faceNormal.x);
	float nDY = abs(faceNormal.y);
	float nDZ = abs(faceNormal.z);

	if (nDX > nDY && nDX > nDZ)
	{
		return 0;
	}
	else if (nDY > nDX && nDY > nDZ)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

float4 getAABB(float4 pos[3], float2 pixelDiagonal)
{
	float4 aabb;

	aabb.xy = min(pos[2].xy, min(pos[1].xy, pos[0].xy));
	aabb.zw = max(pos[2].xy, max(pos[1].xy, pos[0].xy));

	// enlarge by half-pixel
	aabb.xy -= pixelDiagonal;
	aabb.zw += pixelDiagonal;

	return aabb;
}

[maxvertexcount(3)]
void main(triangle GeometryInputType input[3], inout TriangleStream<PixelInputType> outStream)
{
	PixelInputType output = (PixelInputType)0;

	float4 pos[3];

	pos[0] = input[0].posWS;
	pos[1] = input[1].posWS;
	pos[2] = input[2].posWS;

	int selectedIndex = CalculateAxis(pos);

	[unroll(3)]
	for (int i = 0; i < 3; i++)
	{
		// to voxel volume space
		pos[i] = pos[i] - voxelizationPassCBuffer.posWSOffset;
		pos[i].w = 1;

		// project along the dominant axis
		[flatten]
		if (selectedIndex == 0)
		{
			pos[i].xyz = pos[i].zyx;
		}
		else if (selectedIndex == 1)
		{
			pos[i].xyz = pos[i].xzy;
		}

		// normalize
		pos[i].xyz /= voxelizationPassCBuffer.volumeSize.xyz;
		output.posCS_orig = pos[i];

		// for rasterization set z to 1
		pos[i].z = 1;
		output.posCS = pos[i];

		output.AABB = getAABB(pos, float2(1.0 / 64.0, 1.0 / 64.0));

		outStream.Append(output);
	}

	outStream.RestartStrip();
}