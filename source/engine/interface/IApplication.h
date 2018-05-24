#pragma once
#include "IObject.h"

class IApplication : public IObject
{
public:
	virtual ~IApplication() {};

	virtual void update() = 0;
};

