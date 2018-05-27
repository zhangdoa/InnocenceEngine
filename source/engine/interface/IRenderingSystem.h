#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "entity/BaseGraphicPrimitiveHeader.h"

class IRenderingSystem : public ISystem
{
public:
	virtual ~IRenderingSystem() {};

	virtual void setWindowName(const std::string& windowName) = 0;
	virtual void render() = 0;
	virtual bool canRender() = 0;
};