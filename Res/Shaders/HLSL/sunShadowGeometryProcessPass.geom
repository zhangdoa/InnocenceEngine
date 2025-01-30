// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float2 texCoord : TEXCOORD;
	uint rtvId : SV_RenderTargetArrayIndex;
};

[[vk::binding(1, 0)]]
cbuffer CSMCBuffer : register(b1)
{
	CSM_CB CSMs[NR_CSM_SPLITS];
};

[maxvertexcount(3 * NR_CSM_SPLITS)]
void main(triangle GeometryInputType input[3], inout TriangleStream<PixelInputType> outStream)
{
	[unroll(NR_CSM_SPLITS)]
	for (int CSMSplitIndex = 0; CSMSplitIndex < NR_CSM_SPLITS; CSMSplitIndex++)
	{
		[unroll(3)]
		for (int i = 0; i < 3; ++i)
		{
			PixelInputType output;
			output.rtvId = CSMSplitIndex;
			output.posCS = mul(input[i].posWS, CSMs[CSMSplitIndex].v);
			output.posCS = mul(output.posCS, CSMs[CSMSplitIndex].p);
			output.posCS /= output.posCS.w;
			output.texCoord = input[i].texCoord;
			outStream.Append(output);
		}

		outStream.RestartStrip();
	}
}