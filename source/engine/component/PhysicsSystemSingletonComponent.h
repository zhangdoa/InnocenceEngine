#pragma once
#include "../common/InnoType.h"
#include "../common/Componentheaders.h"

#include "../common/InnoConcurrency.h"

struct cullingDataPack
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	EntityID visibleComponentEntityID;
	EntityID MDCEntityID;
	visiblilityType visiblilityType;
};

class PhysicsSystemSingletonComponent
{
public:
	~PhysicsSystemSingletonComponent() {};

	static PhysicsSystemSingletonComponent& getInstance()
	{
		static PhysicsSystemSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<EntityID> m_selectedMDCs;
	std::vector<cullingDataPack> m_cullingDataPack;

	ThreadSafeQueue<VisibleComponent*> m_uninitializedVisibleComponents;
private:
	PhysicsSystemSingletonComponent() {};
};
