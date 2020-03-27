// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posCS : SV_POSITION;
	float4 color : COLOR;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 color : COLOR;
};

[maxvertexcount(36)]
void main(point GeometryInputType input[1], inout TriangleStream<PixelInputType> outStream)
{
	const float4 cubeVertices[8] =
	{
		float4(0.5f, 0.5f, 0.5f, 0.0f),
		float4(0.5f, -0.5f, 0.5f, 0.0f),
		float4(-0.5f, -0.5f, 0.5f, 0.0f),
		float4(-0.5f, 0.5f, 0.5f, 0.0f),
		float4(0.5f, 0.5f, -0.5f, 0.0f),
		float4(0.5f, -0.5f, -0.5f, 0.0f),
		float4(-0.5f, -0.5f, -0.5f, 0.0f),
		float4(-0.5f, 0.5f, -0.5f, 0.0f)
	};

	const int cubeIndices[36] =
	{
		0, 3, 1, 1, 3, 2,
		4, 0, 5, 5, 0, 1,
		7, 4, 6, 6, 4, 5,
		3, 7, 2, 2, 7, 6,
		7, 0, 4, 0, 7, 3,
		1, 2, 5, 5, 2, 6
	};

	float4 projectedVertices[8];

	for (int i = 0; i < 8; ++i)
	{
		projectedVertices[i] = projectedVertices[i] + cubeVertices[i] * 2.0;
		projectedVertices[i] = mul(projectedVertices[i], perFrameCBuffer.v);
		projectedVertices[i] = mul(projectedVertices[i], perFrameCBuffer.p_original);
	}

	for (int t = 0; t < 12; ++t)
	{
		for (int v = 0; v < 3; ++v)
		{
			PixelInputType output = (PixelInputType)0;

			output.posCS = projectedVertices[cubeIndices[t * 3 + v]];
			output.color = input[0].color;

			outStream.Append(output);
		}
	}

	outStream.RestartStrip();
}