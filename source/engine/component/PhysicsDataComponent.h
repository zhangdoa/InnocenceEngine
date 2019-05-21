#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

class PhysicsDataComponent
{
public:
	PhysicsDataComponent() {};
	~PhysicsDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;

	AABB m_AABB;
	Sphere m_sphere;
};
