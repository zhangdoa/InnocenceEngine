#pragma once
#include "../common/InnoMath.h"
#include "../common/InnoType.h"
#include "../common/config.h"

class BaseComponent
{
public:
	BaseComponent() :
		m_parentEntity() {};
	virtual ~BaseComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;
};