#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"

class ITimeSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITimeSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual const TimeData getCurrentTime(unsigned int timezone_adjustment = 8) = 0;
	virtual const long long getCurrentTimeFromEpoch() = 0;
};
