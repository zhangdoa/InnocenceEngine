#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "common/config.h"

class IVisionSystem : public ISystem
{
public:
	virtual ~IVisionSystem() {};

	virtual void setWindowName(const std::string& windowName) = 0;
#if defined(INNO_PLATFORM_WIN32) || defined(INNO_PLATFORM_WIN64)
	virtual void setup(void* appInstance, char* commandLineArg, int showMethod) = 0;
#endif
};