#pragma once
#include "IObject.hpp"

class IApplication : public IObject
{
public:
	virtual ~IApplication() {};

	virtual void update() = 0;
};

