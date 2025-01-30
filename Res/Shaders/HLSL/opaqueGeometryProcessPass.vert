// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION_ORIG;
	float4 posCS_prev : POSITION_PREV;
	float3 posWS : POSITION;
	float2 texCoord : TEXCOORD;
	float3 normalWS : NORMAL;
	float3 tangentWS : TANGENT;
};

[[vk::binding(0, 0)]]
cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(1, 0)]]
cbuffer PerFrameConstantBuffer : register(b1)
{
    PerFrame_CB g_Frame;
}

[[vk::binding(0, 1)]]
StructuredBuffer<PerObject_CB> g_Objects : register(t0);

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	PerObject_CB perObjectCB = g_Objects[m_ObjectIndex];

	float4 posLS = float4(input.posLS.xyz, 1.0f);
	float4 posWS = mul(posLS, perObjectCB.m);
	float4 posVS = mul(posWS, g_Frame.v);
	output.posCS_orig = mul(posVS, g_Frame.p_original);

	float4 posWS_prev = mul(posLS, perObjectCB.m_prev);
	float4 posVS_prev = mul(posWS_prev, g_Frame.v_prev);
	output.posCS_prev = mul(posVS_prev, g_Frame.p_original);

	output.posCS = mul(posVS, g_Frame.p_jittered);

	output.posWS = posWS.xyz;
	output.texCoord = input.texCoord;
	output.normalWS = mul(float4(input.normalLS, 0.0f), perObjectCB.normalMat).xyz;
	output.tangentWS = mul(float4(input.tangentLS, 0.0f), perObjectCB.normalMat).xyz;

	return output;
}