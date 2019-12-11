// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float UUID : ID;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float distanceVS : DISTANCE;
	float UUID : ID;
	uint rtvId : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle GeometryInputType input[3], inout TriangleStream<PixelInputType> outStream)
{
	[unroll(6)]
	for (int face = 0; face < 6; face++)
	{
		PixelInputType output = (PixelInputType)0;
		output.rtvId = face;

		[unroll(3)]
		for (int i = 0; i < 3; ++i)
		{
			output.posCS = mul(input[i].posWS, GICBuffer.t);
			output.posCS = mul(output.posCS, GICBuffer.r[face]);
			output.distanceVS = length(output.posCS.xyz);
			output.posCS = mul(output.posCS, GICBuffer.p);
			output.UUID = input[i].UUID;
			outStream.Append(output);
		}

		outStream.RestartStrip();
	}
}