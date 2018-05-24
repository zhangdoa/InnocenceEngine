#pragma once
#include "IObject.h"

class ISystem : public IObject
{
public:
	virtual ~ISystem() {};

	virtual void update() = 0;
};