#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"

struct Bone
{
	uint32_t m_ID;
	Vec4 m_Pos;
	Vec4 m_Rot;
};

class SkeletonDataComponent : public InnoComponent
{
public:
	Mat4 m_RootOffsetMatrix;
	std::vector<Bone> m_Bones;
};
