#pragma once
#include "../../common/InnoType.h"
#include "../../exports/LowLevelSystem_Export.h"

INNO_INTERFACE InnoTimeSystem
{
public:
	InnoLowLevelSystem_EXPORT virtual bool setup() = 0;
	InnoLowLevelSystem_EXPORT virtual bool initialize() = 0;
	InnoLowLevelSystem_EXPORT virtual bool update() = 0;
	InnoLowLevelSystem_EXPORT virtual bool terminate() = 0;

	InnoLowLevelSystem_EXPORT virtual const long long getGameStartTime() = 0;
	InnoLowLevelSystem_EXPORT virtual const long long getDeltaTime() = 0;
	InnoLowLevelSystem_EXPORT virtual const long long getCurrentTime() = 0;
	InnoLowLevelSystem_EXPORT virtual const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z) = 0;
	InnoLowLevelSystem_EXPORT virtual const std::string getCurrentTimeInLocal(unsigned int timezone_adjustment = 8) = 0;
	InnoLowLevelSystem_EXPORT virtual const std::string getCurrentTimeInLocalForOutput(unsigned int timezone_adjustment = 8) = 0;

	InnoLowLevelSystem_EXPORT virtual objectStatus getStatus() = 0;
};
