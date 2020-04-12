#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"

// w component of m_L2BPos is the bone ID
struct BoneData
{
	Vec4 m_L2BPos;
	Vec4 m_L2BRot;
	Vec4 m_B2PPos;
	Vec4 m_B2PRot;
};

class SkeletonDataComponent : public InnoComponent
{
public:
	Array<BoneData> m_BoneData;
};
