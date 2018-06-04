#pragma once
#include "common/stdafx.h"
#include "ISystem.h"

class IRenderingSystem : public ISystem
{
public:
	virtual ~IRenderingSystem() {};

	virtual void setWindowName(const std::string& windowName) = 0;
};