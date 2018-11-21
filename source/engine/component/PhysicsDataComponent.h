#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "MeshDataComponent.h"

struct physicsData
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

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<physicsData> m_physicsDatas;
};
