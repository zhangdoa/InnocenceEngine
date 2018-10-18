#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoMath.h"
#include "../../exports/LowLevelSystem_Export.h"

namespace InnoLogSystem
{
	InnoLowLevelSystem_EXPORT void setup();
	InnoLowLevelSystem_EXPORT void initialize();
	InnoLowLevelSystem_EXPORT void update();
	InnoLowLevelSystem_EXPORT void shutdown();

	InnoLowLevelSystem_EXPORT void printLog(const std::string& logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(double logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(const vec2& logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(const vec4& logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(const mat4& logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(const std::string& logMessage);

	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};
