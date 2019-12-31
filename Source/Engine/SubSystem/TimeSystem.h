#pragma once
#include "../Interface/ITimeSystem.h"

class InnoTimeSystem : public ITimeSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTimeSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	const TimeData getCurrentTime(uint32_t timezone_adjustment = 8) override;
	const int64_t getCurrentTimeFromEpoch() override;
};
