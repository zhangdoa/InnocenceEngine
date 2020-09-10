#pragma once
#include "InnoType.h"

class InnoObject
{
public:
	InnoObject() = default;
	~InnoObject() = default;

	uint64_t m_UUID = 0;

	bool m_Serializable = false;
	ObjectLifespan m_ObjectLifespan = ObjectLifespan::Invalid;
	ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
};
