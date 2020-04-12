// shadertype=hlsl
#include "common/common.hlsl"

// w is the bone ID
struct AnimationChannelInfo_SB
{
	uint keyOffset;
	uint numKeys;
};

// w is the bone ID
struct AnimationKeyData_SB
{
	float4 pos;
	float4 rot;
};

struct BoneData_SB
{
	float4 L2BPos;
	float4 L2BRot;
	float4 B2PPos;
	float4 B2PRot;
};

StructuredBuffer<AnimationChannelInfo_SB> animationChannelInfoSBuffer : register(t5);
StructuredBuffer<AnimationKeyData_SB> animationKeyDataSBuffer : register(t6);
StructuredBuffer<BoneData_SB> skeletonKeyDataSBuffer : register(t7);

struct VertexInputType
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
	float2 pada : PADA;
	float4 normal : NORMAL;
	float4 padb : PADB;
};

struct PixelInputType
{
	float4 frag_ClipSpacePos : SV_POSITION;
	float4 frag_ClipSpacePos_orig : POSITION_ORIG;
	float4 frag_ClipSpacePos_prev : POSITION_PREV;
	float3 frag_WorldSpacePos : POSITION;
	float2 frag_TexCoord : TEXCOORD;
	float3 frag_Normal : NORMAL;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	int boneID1 = int(input.pada.x);
	float weight1 = input.pada.y;

	float4 L2BPos = skeletonKeyDataSBuffer[boneID1].L2BPos;
	float4x4 L2BPosM =
	{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		L2BPos.x, L2BPos.y, L2BPos.z, 1.0,
	};
	float4 frag_LocalSpacePos = mul(input.position, L2BPos);

	int keyBeginIndex = animationChannelInfoSBuffer[boneID1].keyOffset;
	int numKeys = animationChannelInfoSBuffer[boneID1].numKeys;
	float normalizedCurrentTime = animationPassCBuffer.currentTime;
	float keyIndexInFloat = normalizedCurrentTime * numKeys;
	int index1 = int(trunc(keyIndexInFloat));
	int index2 = index1 + 1;
	if (index1 == numKeys)
	{
		index2 = 0;
	}
	float4 pos1 = animationKeyDataSBuffer[keyBeginIndex + index1].pos;
	float4 pos2 = animationKeyDataSBuffer[keyBeginIndex + index2].pos;
	float weightPos1 = keyIndexInFloat - (float)index1;
	float weightPos2 = 1.0f - weightPos1;

	float4 pos = weightPos1 * pos1 + weightPos2 * pos2;

	float4x4 posM =
	{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		pos1.x, pos1.y, pos1.z, 1.0,
	};
	frag_LocalSpacePos = mul(frag_LocalSpacePos, posM);
	float4 frag_WorldSpacePos = mul(frag_LocalSpacePos, perObjectCBuffer.m);
	float4 frag_CameraSpacePos = mul(frag_WorldSpacePos, perFrameCBuffer.v);
	output.frag_ClipSpacePos_orig = mul(frag_CameraSpacePos, perFrameCBuffer.p_original);

	float4 frag_WorldSpacePos_prev = mul(input.position, perObjectCBuffer.m_prev);
	float4 frag_CameraSpacePos_prev = mul(frag_WorldSpacePos_prev, perFrameCBuffer.v_prev);
	output.frag_ClipSpacePos_prev = mul(frag_CameraSpacePos_prev, perFrameCBuffer.p_original);

	output.frag_ClipSpacePos = mul(frag_CameraSpacePos, perFrameCBuffer.p_jittered);

	output.frag_WorldSpacePos = frag_WorldSpacePos.xyz;
	output.frag_TexCoord = input.texcoord;
	output.frag_Normal = mul(input.normal, perObjectCBuffer.normalMat).xyz;

	return output;
}