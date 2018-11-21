#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "MeshDataComponent.h"

using physicsDataMap = std::unordered_map<MeshDataComponent*, AABB>;

class PhysicsDataComponent
{
public:
	PhysicsDataComponent() {};
	~PhysicsDataComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	physicsDataMap m_physicsDataMap;
};
