// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	uint rtvId : SV_RenderTargetArrayIndex;
};

[maxvertexcount(3 * NR_CSM_SPLITS)]
void main(triangle GeometryInputType input[3], inout TriangleStream<PixelInputType> outStream)
{
	[unroll(NR_CSM_SPLITS)]
	for (int CSMSplitIndex = 0; CSMSplitIndex < NR_CSM_SPLITS; CSMSplitIndex++)
	{
		PixelInputType output = (PixelInputType)0;
		output.rtvId = CSMSplitIndex;

		[unroll(3)]
		for (int i = 0; i < 3; ++i)
		{
			output.position = input[i].position;
			output.texcoord = input[i].texcoord;
			outStream.Append(output);
		}

		outStream.RestartStrip();
	}
}