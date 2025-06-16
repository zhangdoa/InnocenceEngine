// shadertype=hlsl
#include "common/common.hlsl"

// w is the bone ID
struct AnimationKeyData_SB
{
	float4x4 m;
};

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

[[vk::binding(3, 0)]]
cbuffer AnimationPassConstantBuffer : register(b3)
{
    AnimationPass_CB animationPassCBuffer;
}

[[vk::binding(0, 1)]]
StructuredBuffer<PerObject_CB> g_Objects : register(t0);

[[vk::binding(1, 1)]]
StructuredBuffer<PerObject_CB> g_ObjectsPrev : register(t1);

[[vk::binding(5, 1)]]
StructuredBuffer<AnimationKeyData_SB> animationKeyDataSBuffer : register(t5);

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

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float normalizedCurrentTime = animationPassCBuffer.currentTime;
	float currentTickInFloat = normalizedCurrentTime * animationPassCBuffer.numTicks;
	int currentTickInInt = int(trunc(currentTickInFloat));
	int currentTickGlobalIndex = currentTickInInt * animationPassCBuffer.numChannels;

	int boneID1 = int(input.pad1.x);
	float weight1 = input.pad1.y;
	int boneID2 = int(input.pad1.z);
	float weight2 = input.pad1.w;

	float4x4 m1 = animationKeyDataSBuffer[currentTickGlobalIndex + boneID1].m;
	float4x4 m2 = animationKeyDataSBuffer[currentTickGlobalIndex + boneID2].m;

	float4x4 m = m1 * weight1 + m2 * weight2;

	float4 posBS = float4(input.posLS.xyz, 1.0f);
	float4 posLS = mul(posBS, m);

	PerObject_CB perObjectCBuffer = g_Objects[m_ObjectIndex];
	PerObject_CB perObjectCBufferPrev = g_ObjectsPrev[m_ObjectIndex];

	float4 posWS = mul(posLS, perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS_orig = mul(posVS, perFrameCBuffer.p_original);

	float4 posWS_prev = mul(posLS, perObjectCBufferPrev.m);
	float4 posVS_prev = mul(posWS_prev, perFrameCBufferPrev.v);
	output.posCS_prev = mul(posVS_prev, perFrameCBufferPrev.p_original);

	output.posCS = mul(posVS, perFrameCBuffer.p_jittered);

	output.posWS = posWS.xyz;
	output.texCoord = input.texCoord;
	output.normalWS = mul(float4(input.normalLS, 0.0f), perObjectCBuffer.normalMat).xyz;
	output.tangentWS = mul(float4(input.tangentLS, 0.0f), perObjectCBuffer.normalMat).xyz;

	return output;
}