#pragma once
#include "../../common/InnoType.h"
#include "../../exports/LowLevelSystem_Export.h"

namespace InnoTimeSystem
{
	InnoLowLevelSystem_EXPORT void setup();
	InnoLowLevelSystem_EXPORT void initialize();
	InnoLowLevelSystem_EXPORT void update();
	InnoLowLevelSystem_EXPORT void shutdown();

	InnoLowLevelSystem_EXPORT const long long getGameStartTime();
	InnoLowLevelSystem_EXPORT const long long getDeltaTime();
	InnoLowLevelSystem_EXPORT const long long getCurrentTime();
	InnoLowLevelSystem_EXPORT const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z);
	InnoLowLevelSystem_EXPORT const std::string getCurrentTimeInLocal(unsigned int timezone_adjustment = 8);
	InnoLowLevelSystem_EXPORT const std::string getCurrentTimeInLocalForOutput(unsigned int timezone_adjustment = 8);

	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};
