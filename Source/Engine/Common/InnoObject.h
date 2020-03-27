#pragma once
#include "InnoType.h"

class InnoObject
{
public:
	InnoObject() = default;
	~InnoObject() = default;

	uint64_t m_UUID = 0;

	ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	ObjectSource m_ObjectSource = ObjectSource::Invalid;
	ObjectOwnership m_ObjectOwnership = ObjectOwnership::Invalid;
};
