#pragma once
#include "InnoEntity.h"
#include "InnoType.h"

class InnoComponent : public InnoObject
{
public:
	InnoComponent() = default;
	~InnoComponent() = default;

	InnoEntity* m_ParentEntity = 0;
	ComponentType m_ComponentType;
	ObjectName m_Name;
};