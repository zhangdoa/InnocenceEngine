#pragma once
#include "../../Common/InnoType.h"

#include "../../Common/InnoClassTemplate.h"

INNO_INTERFACE IRayTracer
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRayTracer);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool Update() = 0;
	virtual bool Terminate() = 0;

	virtual ObjectStatus GetStatus() = 0;
};