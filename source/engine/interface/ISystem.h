#pragma once
#include "IObject.h"

class ISystem : public IObject
{
public:
	virtual ~ISystem() {};

	// setup() only sets static member data
	virtual void setup() = 0;
	// initialize() only excutes dynamic function invocation
	virtual void initialize() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;
};