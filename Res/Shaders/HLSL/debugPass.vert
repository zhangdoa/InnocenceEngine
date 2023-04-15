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

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	uint materialID : MATERIALID;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.materialID = debugMeshSBuffer[input.instanceId].materialID;
	output.posCS = mul(float4(input.posLS, 1.0f), debugMeshSBuffer[input.instanceId].m);
	output.posCS = mul(output.posCS, perFrameCBuffer.v);
	output.posCS = mul(output.posCS, perFrameCBuffer.p_jittered);

	return output;
}