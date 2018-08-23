#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "common/config.h"

class IVisionSystem : public ISystem
{
public:
	virtual ~IVisionSystem() {};

#if defined(INNO_RENDERER_DX)
	virtual void setup(void* appInstance, char* commandLineArg, int showMethod) = 0;
#endif
};