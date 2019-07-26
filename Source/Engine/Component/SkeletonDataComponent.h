#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"

struct Bone
{
	unsigned int m_ID;
	vec4 m_Pos;
	vec4 m_Rot;
};

class SkeletonDataComponent : public InnoComponent
{
public:
	mat4 m_RootOffsetMatrix;
	std::vector<Bone> m_Bones;
};
