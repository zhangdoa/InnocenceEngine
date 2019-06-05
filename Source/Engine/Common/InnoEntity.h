#pragma once
#include "InnoType.h"

class InnoEntity
{
public:
	InnoEntity() = default;
	~InnoEntity() = default;

	EntityID m_entityID;
	EntityName m_entityName;

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	ObjectSource m_objectSource = ObjectSource::Runtime;
	ObjectUsage m_objectUsage = ObjectUsage::Gameplay;
};