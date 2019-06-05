#pragma once
#include "../common/InnoComponent.h"
#include "../common/InnoMath.h"

class PhysicsDataComponent : public InnoComponent
{
public:
	PhysicsDataComponent() {};
	~PhysicsDataComponent() {};

	AABB m_AABB;
	Sphere m_sphere;
};
