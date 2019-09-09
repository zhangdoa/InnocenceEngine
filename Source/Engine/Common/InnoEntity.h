#pragma once
#include "InnoType.h"

class InnoEntity
{
public:
	InnoEntity() = default;
	~InnoEntity() = default;

	EntityID m_EntityID;
	EntityName m_EntityName;

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	ObjectSource m_ObjectSource = ObjectSource::Runtime;
	ObjectOwnership m_ObjectOwnership = ObjectOwnership::Client;
};
