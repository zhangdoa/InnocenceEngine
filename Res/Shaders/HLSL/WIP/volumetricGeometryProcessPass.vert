// shadertype=hlsl
#include "common/common.hlsl"

[[vk::binding(0, 0)]]
cbuffer RootConstants : register(b0)
{
	uint m_ObjectIndex;
};

[[vk::binding(1, 0)]]
cbuffer PerFrameConstantBuffer : register(b1)
{
    PerFrame_CB perFrameCBuffer;
}

[[vk::binding(2, 0)]]
cbuffer PerFrameConstantBufferPrev : register(b2)
{
    PerFrame_CB perFrameCBufferPrev;
}

[[vk::binding(0, 1)]]
StructuredBuffer<PerObject_CB> g_Objects : register(t0);

[[vk::binding(1, 1)]]
StructuredBuffer<PerObject_CB> g_ObjectsPrev : register(t1);

struct GeometryInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION_ORIG;
	float4 posCS_prev : POSITION_PREV;
	float2 texCoord : TEXCOORD;
};

GeometryInputType main(VertexInputType input)
{
	GeometryInputType output;

	PerObject_CB perObjectCBuffer = g_Objects[m_ObjectIndex];
	PerObject_CB perObjectCBufferPrev = g_ObjectsPrev[m_ObjectIndex];

	float4 posWS = mul(float4(input.posLS, 1.0f), perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS_orig = mul(posVS, perFrameCBuffer.p_original);
	output.posCS_orig /= output.posCS_orig.w;

	float4 posWS_prev = mul(float4(input.posLS, 1.0f), perObjectCBufferPrev.m);
	float4 posVS_prev = mul(posWS_prev, perFrameCBufferPrev.v);
	output.posCS_prev = mul(posVS_prev, perFrameCBufferPrev.p_original);
	output.posCS_prev /= output.posCS_prev.w;

	output.posCS = mul(posVS, perFrameCBuffer.p_jittered);

	float expDepth = -posVS.z / (perFrameCBuffer.zFar - perFrameCBuffer.zNear);
	expDepth = 1.0 - exp(-expDepth * 8);

	float expDepth_prev = -posVS_prev.z / (perFrameCBuffer.zFar - perFrameCBuffer.zNear);
	expDepth_prev = 1.0 - exp(-expDepth_prev * 8);

	output.posCS_orig.w = expDepth;
	output.posCS_prev.w = expDepth_prev;

	output.texCoord = input.texCoord;

	return output;
}