// shadertype=hlsl
#include "common/common.hlsl"

struct GeometryInputType
{
	float4 posWS : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

[[vk::binding(0, 0)]]
cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(0, 1)]]
StructuredBuffer<PerObject_CB> g_Objects : register(t0);

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	PerObject_CB perObjectCB = g_Objects[m_ObjectIndex];

	output.posWS = mul(float4(input.posLS, 1.0f), perObjectCB.m);
	output.texCoord = input.texCoord;

	return output;
}