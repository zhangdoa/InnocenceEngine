#pragma once
#include "ISystem.h"
#include "config.h"

class IApplication : public ISystem
{
public:
	virtual ~IApplication() {};

#if defined(INNO_RENDERER_DX)
	virtual void setup(void* appInstance, char* commandLineArg, int showMethod) = 0;
#endif
};

