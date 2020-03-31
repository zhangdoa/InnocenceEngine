#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"

// w component of position is the bone ID
struct Bone
{
	Vec4 m_Pos;
	Vec4 m_Rot;
};

class SkeletonDataComponent : public InnoComponent
{
public:
	Mat4 m_RootOffsetMatrix;
	Array<Bone> m_Bones;
};
