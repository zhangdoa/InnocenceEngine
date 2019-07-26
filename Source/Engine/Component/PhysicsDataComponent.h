#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"

class PhysicsDataComponent : public InnoComponent
{
public:
	AABB m_AABB = {};
	Sphere m_sphere = {};
};
