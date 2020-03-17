// shadertype=hlsl
#include "common/common.hlsl"

struct VertexInputType
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	float2 pada : PADA;
	float4 normal : NORMAL;
	float4 padb : PADB;
	uint vertexId : SV_VertexID;
};

struct GeometryInputType
{
	float4 posCS : SV_POSITION;
};

Texture3D<float4> in_volumetricPassRT0 : register(t0);

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	float3 position = float3
	(
		input.vertexId % 64,
		(input.vertexId / 64) % 64,
		input.vertexId / (64 * 64)
	);

	int3 texPos = int3(position);
	output.posCS = in_volumetricPassRT0[texPos];

	return output;
}