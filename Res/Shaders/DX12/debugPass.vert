// shadertype=hlsl
#include "common/common.hlsl"

StructuredBuffer<matrix> debugSBuffer : register(t13);

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
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	output.posCS = mul(input.position, debugSBuffer[input.instanceId]);
	output.posCS = mul(output.posCS, cameraCBuffer.t);
	output.posCS = mul(output.posCS, cameraCBuffer.r);
	output.posCS = mul(output.posCS, cameraCBuffer.p_jittered);

	return output;
}