#pragma once
#include "interface/IGraphicPrimitive.h"

class IRenderPass : public IGraphicPrimitive
{
public:
	IRenderPass() {};
	virtual ~IRenderPass() {};

	virtual void draw() = 0;
};
