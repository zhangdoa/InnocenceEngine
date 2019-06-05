#pragma once
#include "../common/InnoComponent.h"
#include "../common/InnoMath.h"

struct Key
{
	double m_Time;
	vec4 m_Pos;
	vec4 m_Rot;
};

class AnimationDataComponent : public InnoComponent
{
public:
	AnimationDataComponent() {};
	~AnimationDataComponent() {};

	std::vector<Key> m_Keys;
};
