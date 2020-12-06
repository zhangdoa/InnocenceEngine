// shadertype=hlsl
#include "common/common.hlsl"

struct DebugMeshData
{
	matrix m;
	uint materialID;
	uint padding[15];
};

[[vk::binding(0, 1)]]
StructuredBuffer<DebugMeshData> debugMeshSBuffer : register(t0);

struct VertexInputType
{
	float4 posLS : POSITION;
	float2 texCoord : TEXCOORD;
	float2 pada : PADA;
	float4 normalLS : NORMAL;
	float4 padb : PADB;
	uint instanceId : SV_InstanceID;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	uint materialID : MATERIALID;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.materialID = debugMeshSBuffer[input.instanceId].materialID;
	output.posCS = mul(input.posLS, debugMeshSBuffer[input.instanceId].m);
	output.posCS = mul(output.posCS, perFrameCBuffer.v);
	output.posCS = mul(output.posCS, perFrameCBuffer.p_jittered);

	return output;
}