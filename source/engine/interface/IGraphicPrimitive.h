#pragma once
#include "IObject.hpp"

class IGraphicPrimitive : public IObject
{
public:
	virtual ~IGraphicPrimitive() {};

	virtual void update() = 0;
};