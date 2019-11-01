// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float4 normal : NORMAL;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION;
	float2 texcoord : TEXCOORD;
	float4 normal : NORMAL;
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
			output.posCS = mul(output.posWS, GICamera_t);
			output.posCS = mul(output.posCS, GICamera_r[face]);
			output.posCS = mul(output.posCS, GICamera_p);
			output.texcoord = input[i].texcoord;
			output.normal = input[i].normal;
			outStream.Append(output);
		}

		outStream.RestartStrip();
	}
}