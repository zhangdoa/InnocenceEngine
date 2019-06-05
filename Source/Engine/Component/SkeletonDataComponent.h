#pragma once
#include "../common/InnoComponent.h"
#include "../common/InnoMath.h"

struct Bone
{
	unsigned int m_ID;
	vec4 m_Pos;
	vec4 m_Rot;
};

class SkeletonDataComponent : public InnoComponent
{
public:
	SkeletonDataComponent() {};
	~SkeletonDataComponent() {};

	std::vector<Bone> m_Bones;
};
