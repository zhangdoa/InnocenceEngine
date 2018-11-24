#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "MeshDataComponent.h"

struct PhysicsData
{
	MeshDataComponent* MDC;
	MeshDataComponent* wireframeMDC;
	AABB AABB;
};

class PhysicsDataComponent
{
public:
	PhysicsDataComponent() {};
	~PhysicsDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	std::vector<PhysicsData> m_physicsDatas;
};
