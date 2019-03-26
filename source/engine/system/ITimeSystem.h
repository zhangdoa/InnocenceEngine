#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"

INNO_INTERFACE ITimeSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITimeSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual const long long getDeltaTime() = 0;
	virtual const TimeData getCurrentTime(unsigned int timezone_adjustment = 8) = 0;
};
