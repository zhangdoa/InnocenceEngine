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
StructuredBuffer<Transform_CB> g_Transforms : register(t0);

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	Transform_CB transformCB = g_Transforms[m_ObjectIndex];

	output.posWS = mul(float4(input.posLS, 1.0f), transformCB.m);
	output.texCoord = input.texCoord;

	return output;
}