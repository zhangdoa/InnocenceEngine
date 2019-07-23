#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"

class IRayTracer
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRayTracer);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool Execute() = 0;
	virtual bool Terminate() = 0;

	virtual ObjectStatus GetStatus() = 0;
};