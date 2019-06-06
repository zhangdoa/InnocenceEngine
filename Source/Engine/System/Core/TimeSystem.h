#pragma once
#include "ITimeSystem.h"

class InnoTimeSystem : INNO_IMPLEMENT ITimeSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTimeSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	const TimeData getCurrentTime(unsigned int timezone_adjustment = 8) override;
	const long long getCurrentTimeFromEpoch() override;
};
