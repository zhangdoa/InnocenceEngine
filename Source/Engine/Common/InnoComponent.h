#pragma once
#include "InnoType.h"
#include "InnoEntity.h"

class InnoComponent
{
public:
	InnoComponent() = default;
	~InnoComponent() = default;

	InnoEntity* m_parentEntity;
	ComponentName m_componentName;
	unsigned int m_UUID = 0;
	ComponentType m_ComponentType;
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	ObjectSource m_objectSource = ObjectSource::Runtime;
	ObjectUsage m_objectUsage = ObjectUsage::Gameplay;
};