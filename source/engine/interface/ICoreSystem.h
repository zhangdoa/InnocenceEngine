#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "common/config.h"

class ICoreSystem : public ISystem
{
public:
	virtual ~ICoreSystem() {};

#if defined(INNO_PLATFORM_WIN32) || defined(INNO_PLATFORM_WIN64)
	virtual void setup(void* appInstance, char* commandLineArg, int showMethod) = 0;
#endif
};