#pragma once
#include "IObject.hpp"

class ISystem : public IObject
{
public:
	virtual ~ISystem() {};

	virtual void update() = 0;
};