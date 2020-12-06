// shadertype=hlsl
#include "common/common.hlsl"

// w is the bone ID
struct AnimationKeyData_SB
{
	float4x4 m;
};

StructuredBuffer<AnimationKeyData_SB> animationKeyDataSBuffer : register(t5);

struct VertexInputType
{
	float4 posBS : POSITION;
	float2 texCoord : TEXCOORD;
	float2 pada : PADA;
	float4 normalLS : NORMAL;
	float4 padb : PADB;
};

struct PixelInputType
{
	float4 posCS : SV_POSITION;
	float4 posCS_orig : POSITION_ORIG;
	float4 posCS_prev : POSITION_PREV;
	float3 posWS : POSITION;
	float2 texCoord : TEXCOORD;
	float3 normalWS : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	float normalizedCurrentTime = animationPassCBuffer.currentTime;
	float currentTickInFloat = normalizedCurrentTime * animationPassCBuffer.numTicks;
	int currentTickInInt = int(trunc(currentTickInFloat));
	int currentTickGlobalIndex = currentTickInInt * animationPassCBuffer.numChannels;

	int boneID1 = int(input.pada.x);
	float weight1 = input.pada.y;
	int boneID2 = int(input.padb.x);
	float weight2 = input.padb.y;
	int boneID3 = int(input.padb.z);
	float weight3 = input.padb.w;
	int boneID4 = int(input.posBS.w);
	float weight4 = input.normalLS.w;

	float4x4 m1 = animationKeyDataSBuffer[currentTickGlobalIndex + boneID1].m;
	float4x4 m2 = animationKeyDataSBuffer[currentTickGlobalIndex + boneID2].m;
	float4x4 m3 = animationKeyDataSBuffer[currentTickGlobalIndex + boneID3].m;
	float4x4 m4 = animationKeyDataSBuffer[currentTickGlobalIndex + boneID4].m;

	float4x4 m = m1 * weight1 + m2 * weight2 + m3 * weight3 + m4 * weight4;

	float4 posBS = float4(input.posBS.xyz, 1.0f);
	float4 posLS = mul(posBS, m);

	float4 posWS = mul(posLS, perObjectCBuffer.m);
	float4 posVS = mul(posWS, perFrameCBuffer.v);
	output.posCS_orig = mul(posVS, perFrameCBuffer.p_original);

	float4 posWS_prev = mul(posLS, perObjectCBuffer.m_prev);
	float4 posVS_prev = mul(posWS_prev, perFrameCBuffer.v_prev);
	output.posCS_prev = mul(posVS_prev, perFrameCBuffer.p_original);

	output.posCS = mul(posVS, perFrameCBuffer.p_jittered);

	output.posWS = posWS.xyz;
	output.texCoord = input.texCoord;
	float4 normalLS = float4(input.normalLS.xyz, 0.0f);
	output.normalWS = mul(input.normalLS, perObjectCBuffer.normalMat).xyz;

	return output;
}