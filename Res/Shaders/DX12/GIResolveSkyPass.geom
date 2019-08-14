// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	uint rtvId : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle GeometryInputType input[3], inout TriangleStream<PixelInputType> outStream)
{
	[unroll(NR_CSM_SPLITS)]
	for (int face = 0; face < 6; face++)
	{
		PixelInputType output = (PixelInputType)0;
		output.rtvId = face;

		[unroll(3)]
		for (int i = 0; i < 3; ++i)
		{
			float4 posWS = float4(-1.0 * input[i].posWS.xyz, 1.0);
			output.posCS = mul(posWS, GI_cam_t);
			output.posCS = mul(output.posCS, GI_cam_r[face]);
			output.posCS = mul(output.posCS, GI_cam_p);
			output.posCS.z = output.posCS.w;
			outStream.Append(output);
		}

		outStream.RestartStrip();
	}
}