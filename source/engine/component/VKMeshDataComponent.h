#pragma once
#include "../common/InnoType.h"

class VKMeshDataComponent
{
public:
	VKMeshDataComponent() {};
	~VKMeshDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;
};

