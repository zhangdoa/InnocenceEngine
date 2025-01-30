// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 normalWS : NORMAL;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION;
	float2 texCoord : TEXCOORD;
	float4 normalWS : NORMAL;
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
			output.posWS = input[i].posWS;
			output.posCS = mul(output.posWS, GICBuffer.t);
			output.posCS = mul(output.posCS, GICBuffer.r[face]);
			output.posCS = mul(output.posCS, GICBuffer.p);
			output.texCoord = input[i].texCoord;
			output.normalWS = input[i].normalWS;
			outStream.Append(output);
		}

		outStream.RestartStrip();
	}
}