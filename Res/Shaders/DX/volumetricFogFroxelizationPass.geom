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
	float4 AABB : AABB;
};

float4 AABB(float4 pos[3], float2 pixelDiagonal)
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

	pos[0] = input[0].posCS / input[0].posCS.w;
	pos[1] = input[1].posCS / input[1].posCS.w;
	pos[2] = input[2].posCS / input[2].posCS.w;

	//// xyz is normal, w is distance
	//float4 trianglePlane;
	//trianglePlane.xyz = cross(pos[1].xyz - pos[0].xyz, pos[2].xyz - pos[0].xyz);
	//trianglePlane.xyz = normalize(trianglePlane.xyz);
	//trianglePlane.w = -dot(pos[0].xyz, trianglePlane.xyz);

	//// change winding, otherwise there are artifacts for the back faces.
	//if (dot(trianglePlane.xyz, float3(0.0, 0.0, 1.0)) < 0.0)
	//{
	//	float4 vertexTemp = pos[2];

	//	pos[2] = pos[1];

	//	pos[1] = vertexTemp;
	//}

	//float2 halfPixel = float2(1.0 / 160.0, 1.0 / 90.0);

	//if (trianglePlane.z == 0.0) return;

	//// calculate the plane through each edge of the triangle
	//// in normal form for dilatation of the triangle
	//float3 planes[3];
	//planes[0] = cross(pos[0].xyw - pos[2].xyw, pos[2].xyw);
	//planes[1] = cross(pos[1].xyw - pos[0].xyw, pos[0].xyw);
	//planes[2] = cross(pos[2].xyw - pos[1].xyw, pos[1].xyw);
	//planes[0].z -= dot(halfPixel, abs(planes[0].xy));
	//planes[1].z -= dot(halfPixel, abs(planes[1].xy));
	//planes[2].z -= dot(halfPixel, abs(planes[2].xy));

	//// calculate intersection between translated planes
	//float3 intersection[3];
	//intersection[0] = cross(planes[0], planes[1]);
	//intersection[1] = cross(planes[1], planes[2]);
	//intersection[2] = cross(planes[2], planes[0]);
	//intersection[0] /= intersection[0].z;
	//intersection[1] /= intersection[1].z;
	//intersection[2] /= intersection[2].z;

	//// calculate dilated triangle vertices
	//float z[3];
	//z[0] = -(intersection[0].x * trianglePlane.x + intersection[0].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	//z[1] = -(intersection[1].x * trianglePlane.x + intersection[1].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	//z[2] = -(intersection[2].x * trianglePlane.x + intersection[2].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	//pos[0].xyz = float3(intersection[0].xy, z[0]);
	//pos[1].xyz = float3(intersection[1].xy, z[1]);
	//pos[2].xyz = float3(intersection[2].xy, z[2]);

	[unroll(3)]
	for (int i = 0; i < 3; ++i)
	{
		output.posCS = input[i].posCS;
		// expanded aabb for triangle
		//output.AABB = AABB(pos, halfPixel);

		outStream.Append(output);
	}

	outStream.RestartStrip();
}