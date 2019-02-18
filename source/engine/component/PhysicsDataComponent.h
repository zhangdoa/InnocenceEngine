#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "MeshDataComponent.h"

struct PhysicsData
{
	MeshDataComponent* MDC;
	MeshDataComponent* wireframeMDC;
	AABB aabb;
	Sphere sphere;
};

class PhysicsDataComponent
{
public:
	PhysicsDataComponent() {};
	~PhysicsDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<PhysicsData> m_physicsDatas;
};
