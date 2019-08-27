// shadertype=hlsl
#include "common/common.hlsl"

struct ProbeMeshData
{
	matrix m;
	float4 index;
	float4 padding[3];
};

StructuredBuffer<ProbeMeshData> probeMeshSBuffer : register(t0);

struct VertexInputType
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	float2 pada : PADA;
	float4 normal : NORMAL;
	float4 padb : PADB;
	uint instanceId : SV_InstanceID;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 probeIndex : PROBE_INDEX;
	float4 normal : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.probeIndex = probeMeshSBuffer[input.instanceId].index;
	output.normal = input.normal;

	output.posCS = mul(input.position, probeMeshSBuffer[input.instanceId].m);
	output.posCS = mul(output.posCS, cameraCBuffer.t);
	output.posCS = mul(output.posCS, cameraCBuffer.r);
	output.posCS = mul(output.posCS, cameraCBuffer.p_original);

	return output;
}