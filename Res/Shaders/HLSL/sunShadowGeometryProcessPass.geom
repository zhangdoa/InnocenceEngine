// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float2 texcoord : TEXCOORD;
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
			output.posCS = mul(input[i].posWS, CSMs[CSMSplitIndex].v);
			output.posCS = mul(output.posCS, CSMs[CSMSplitIndex].p);
			output.texcoord = input[i].texcoord;
			outStream.Append(output);
		}

		outStream.RestartStrip();
	}
}