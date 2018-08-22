#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "common/config.h"

class ICoreSystem : public ISystem
{
public:
	virtual ~ICoreSystem() {};

#if defined(INNO_RENDERER_DX)
	virtual void setup(void* appInstance, char* commandLineArg, int showMethod) = 0;
#endif
};