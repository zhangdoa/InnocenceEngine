#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"

struct BoneData
{
	Mat4 m_L2B;
};

class SkeletonDataComponent : public InnoComponent
{
public:
	Array<BoneData> m_BoneData;
};
