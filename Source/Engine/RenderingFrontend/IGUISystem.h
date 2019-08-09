#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"

class IGUISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGUISystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;
};