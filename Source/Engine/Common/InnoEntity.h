#pragma once
#include "InnoObject.h"

class InnoEntity : public InnoObject
{
public:
	InnoEntity() = default;
	~InnoEntity() = default;

	ObjectName m_Name;
};
