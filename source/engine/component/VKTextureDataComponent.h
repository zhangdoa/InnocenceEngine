#pragma once
#include "../common/InnoType.h"

class VKTextureDataComponent
{
public:
	VKTextureDataComponent() {};
	~VKTextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;
};

