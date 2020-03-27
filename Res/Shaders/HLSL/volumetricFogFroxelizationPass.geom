// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posCS : SV_POSITION;
	float3 posWS : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
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
	PixelInputType output[3];

	float4 pos[3];

	pos[0] = input[0].posCS;
	pos[1] = input[1].posCS;
	pos[2] = input[2].posCS;

	int selectedIndex = CalculateAxis(pos);

	[unroll(3)]
	for (int i = 0; i < 3; i++)
	{
		pos[i] /= pos[i].w;

		output[i].posCS_orig = pos[i];

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

		// for rasterization set z to 1
		pos[i].z = 1;
	}

	// Conservative Rasterization setup:
	float2 side0N = normalize(pos[1].xy - pos[0].xy);
	float2 side1N = normalize(pos[2].xy - pos[1].xy);
	float2 side2N = normalize(pos[0].xy - pos[2].xy);
	const float texelSize = 1.0f / 64.0f;
	pos[0].xy += normalize(-side0N + side2N) * texelSize;
	pos[1].xy += normalize(side0N - side1N) * texelSize;
	pos[2].xy += normalize(side1N - side2N) * texelSize;

	[unroll(3)]
	for (int i = 0; i < 3; i++)
	{
		output[i].posCS = pos[i];

		output[i].AABB = getAABB(pos, float2(texelSize, texelSize));

		outStream.Append(output[i]);
	}

	outStream.RestartStrip();
}