#pragma once
#include "InnoType.h"
#include "InnoEntity.h"

class InnoComponent
{
public:
	InnoComponent() = default;
	~InnoComponent() = default;

	InnoEntity* m_ParentEntity = 0;
	ComponentName m_ComponentName;
	uint64_t m_UUID = 0;
	ComponentType m_ComponentType;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	ObjectSource m_ObjectSource = ObjectSource::Runtime;
	ObjectOwnership m_ObjectOwnership = ObjectOwnership::Client;
};