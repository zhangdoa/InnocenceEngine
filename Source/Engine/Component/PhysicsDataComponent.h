#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"

class PhysicsDataComponent : public InnoComponent
{
public:
	PhysicsDataComponent() {};
	~PhysicsDataComponent() {};

	AABB m_AABB;
	Sphere m_sphere;
};
