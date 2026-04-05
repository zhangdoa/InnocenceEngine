// shadertype=hlsl
#include "common/common.hlsl"

struct ProbeMeshData
{
	matrix m;
	float4 index;
	float4 padding[3];
};

[[vk::binding(0, 1)]]
StructuredBuffer<ProbeMeshData> probeMeshSBuffer : register(t0);

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posWS : POSITION_WS;
	float4 probeIndex : PROBE_INDEX;
	float4 normalWS : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.probeIndex = probeMeshSBuffer[input.instanceId].index;
	output.normalWS = float4(input.normalLS, 0.0f);

	output.posWS = mul(float4(input.posLS, 1.0f), probeMeshSBuffer[input.instanceId].m);
	output.posCS = mul(output.posWS, perFrameCBuffer.v);
	output.posCS = mul(output.posCS, perFrameCBuffer.p_original);

	return output;
}