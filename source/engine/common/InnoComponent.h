#pragma once
#include "InnoType.h"

class InnoComponent
{
public:
	InnoComponent() = default;
	~InnoComponent() = default;

	EntityID m_parentEntity;
	ComponentName m_componentName;
	unsigned int m_UUID = 0;
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	ObjectSource m_objectSource = ObjectSource::Runtime;
	ObjectUsage m_objectUsage = ObjectUsage::Product;
};