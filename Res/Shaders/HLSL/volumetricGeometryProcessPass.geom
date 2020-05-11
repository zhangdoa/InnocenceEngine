// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POS_WS;
	float2 texcoord : TEXCOORD;
	float4 normal : NORMAL;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION;
	nointerpolation float4 AABB : AABB;
	float4 normal : NORMAL;
	float2 texcoord : TEXCOORD;
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
	float4 normal[3];
	float2 texcoord[3];

	[unroll(3)]
	for (int j = 0; j < 3; j++)
	{
		pos[j] = input[j].posWS;
		normal[j] = input[j].normal;
		texcoord[j] = input[j].texcoord;
	}

	int selectedIndex = CalculateAxis(pos);

	[unroll(3)]
	for (int i = 0; i < 3; i++)
	{
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
	}

	// xyz is normal, w is distance
	float4 trianglePlane;
	trianglePlane.xyz = cross(pos[1].xyz - pos[0].xyz, pos[2].xyz - pos[0].xyz);
	trianglePlane.xyz = normalize(trianglePlane.xyz);
	trianglePlane.w = -dot(pos[0].xyz, trianglePlane.xyz);

	// change winding, otherwise there are artifacts for the back faces.
	if (dot(trianglePlane.xyz, float3(0.0, 0.0, 1.0)) < 0.0)
	{
		float4 vertexTemp = pos[2];
		float4 normalTemp = normal[2];
		float2 texcoordTemp = texcoord[2];

		pos[2] = pos[1];
		normal[2] = normal[1];
		texcoord[2] = texcoord[1];

		pos[1] = vertexTemp;
		normal[1] = normalTemp;
		texcoord[1] = texcoordTemp;
	}

	// for rasterization set z to 1
	[unroll(3)]
	for (int i = 0; i < 3; i++)
	{
		pos[i].z = 1;
	}

	[unroll(3)]
	for (int i = 0; i < 3; i++)
	{
		output[i].posCS = pos[i];
		output[i].texcoord = texcoord[i];
		output[i].normal = normal[i];
		output[i].AABB = float4(0.0f, 0.0f, 0.0f, 0.0f);

		outStream.Append(output[i]);
	}

	outStream.RestartStrip();
}