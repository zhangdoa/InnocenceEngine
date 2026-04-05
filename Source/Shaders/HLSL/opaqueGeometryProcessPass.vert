// shadertype=hlsl
#include "common/common.hlsl"

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION_ORIG;
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
StructuredBuffer<Transform_CB> g_Transforms : register(t0);

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	Transform_CB transformCB = g_Transforms[m_ObjectIndex];

	float4 posLS = float4(input.posLS.xyz, 1.0f);
	float4 posWS = mul(posLS, transformCB.m);
	float4 posVS = mul(posWS, g_Frame.v);
	output.posCS_orig = mul(posVS, g_Frame.p_original);

	output.posCS = mul(posVS, g_Frame.p_jittered);

	output.posWS = posWS.xyz;
	output.texCoord = input.texCoord;
	output.normalWS = mul(float4(input.normalLS, 0.0f), transformCB.normalMat).xyz;
	output.tangentWS = mul(float4(input.tangentLS, 0.0f), transformCB.normalMat).xyz;

	return output;
}