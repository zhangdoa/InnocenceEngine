#pragma once
#include "IObject.h"

class IGraphicPrimitive : public IObject
{
public:
	virtual ~IGraphicPrimitive() {};

	virtual void update() = 0;
};