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

	VisibleComponent* m_selectedVisibleComponent;
private:
	PhysicsSystemComponent() {};
};
