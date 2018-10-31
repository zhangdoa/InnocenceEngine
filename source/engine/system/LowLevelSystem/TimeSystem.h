#pragma once
#include "../../common/InnoType.h"
#include "../../exports/LowLevelSystem_Export.h"

namespace InnoTimeSystem
{
	InnoLowLevelSystem_EXPORT bool setup();
	InnoLowLevelSystem_EXPORT bool initialize();
	InnoLowLevelSystem_EXPORT bool update();
	InnoLowLevelSystem_EXPORT bool terminate();

	InnoLowLevelSystem_EXPORT const long long getGameStartTime();
	InnoLowLevelSystem_EXPORT const long long getDeltaTime();
	InnoLowLevelSystem_EXPORT const long long getCurrentTime();
	InnoLowLevelSystem_EXPORT const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z);
	InnoLowLevelSystem_EXPORT const std::string getCurrentTimeInLocal(unsigned int timezone_adjustment = 8);
	InnoLowLevelSystem_EXPORT const std::string getCurrentTimeInLocalForOutput(unsigned int timezone_adjustment = 8);

	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};
