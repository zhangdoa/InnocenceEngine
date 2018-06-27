#pragma once
#include "common/stdafx.h"
#include "ISystem.h"

class IVisionSystem : public ISystem
{
public:
	virtual ~IVisionSystem() {};

	virtual void setWindowName(const std::string& windowName) = 0;
};