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

	virtual const TimeData getCurrentTime(uint32_t timezone_adjustment = 8) = 0;
	virtual const int64_t getCurrentTimeFromEpoch() = 0;
};
