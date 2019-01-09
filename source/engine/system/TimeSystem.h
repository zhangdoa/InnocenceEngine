#pragma once
#include "ITimeSystem.h"

class InnoTimeSystem : INNO_IMPLEMENT ITimeSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTimeSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT const long long getDeltaTime() override;
	INNO_SYSTEM_EXPORT const TimeData getCurrentTime(unsigned int timezone_adjustment = 8) override;
};
