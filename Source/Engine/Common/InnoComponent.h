#pragma once
#include "InnoEntity.h"
#include "InnoType.h"

class InnoComponent : public InnoObject
{
public:
	InnoComponent() = default;
	~InnoComponent() = default;

	InnoEntity* m_ParentEntity = 0;
	uint32_t m_ComponentType = 0;
	ObjectName m_Name;
};