#pragma once
#include "../common/InnoType.h"
#include "../common/ComponentHeaders.h"

#include "../common/InnoConcurrency.h"

struct CullingDataPack
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	EntityID visibleComponentEntityID;
	EntityID MDCEntityID;
	VisiblilityType visiblilityType;
};

struct AABBWireframeDataPack
{
	mat4 m;
	MeshDataComponent* MDC;
};

class PhysicsSystemComponent
{
public:
	~PhysicsSystemComponent() {};

	static PhysicsSystemComponent& get()
	{
		static PhysicsSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::vector<CullingDataPack> m_cullingDataPack;
	std::vector<AABBWireframeDataPack> m_AABBWireframeDataPack;

	ThreadSafeQueue<VisibleComponent*> m_uninitializedVisibleComponents;
private:
	PhysicsSystemComponent() {};
};
