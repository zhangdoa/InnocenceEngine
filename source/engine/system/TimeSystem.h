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

	INNO_SYSTEM_EXPORT const long long getStartTime() override;
	INNO_SYSTEM_EXPORT const long long getDeltaTime() override;
	INNO_SYSTEM_EXPORT const long long getCurrentTime() override;
	INNO_SYSTEM_EXPORT const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z) override;
	INNO_SYSTEM_EXPORT const TimeData getCurrentTimeInLocal(unsigned int timezone_adjustment = 8) override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;
};
