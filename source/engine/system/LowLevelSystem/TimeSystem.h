#pragma once
#include "ITimeSystem.h"

class InnoTimeSystem INNO_IMPLEMENT ITimeSystem
{
	InnoLowLevelSystem_EXPORT bool setup() override;
	InnoLowLevelSystem_EXPORT bool initialize() override;
	InnoLowLevelSystem_EXPORT bool update() override;
	InnoLowLevelSystem_EXPORT bool terminate() override;

	InnoLowLevelSystem_EXPORT const long long getGameStartTime() override;
	InnoLowLevelSystem_EXPORT const long long getDeltaTime() override;
	InnoLowLevelSystem_EXPORT const long long getCurrentTime();
	InnoLowLevelSystem_EXPORT const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z) override;
	InnoLowLevelSystem_EXPORT const std::string getCurrentTimeInLocal(unsigned int timezone_adjustment = 8) override;
	InnoLowLevelSystem_EXPORT const std::string getCurrentTimeInLocalForOutput(unsigned int timezone_adjustment = 8) override;

	InnoLowLevelSystem_EXPORT objectStatus getStatus() override;
};
