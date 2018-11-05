#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoMath.h"
#include "../../exports/LowLevelSystem_Export.h"

namespace InnoLogSystem
{
	InnoLowLevelSystem_EXPORT bool setup();
	InnoLowLevelSystem_EXPORT bool initialize();
	InnoLowLevelSystem_EXPORT bool update();
	InnoLowLevelSystem_EXPORT bool terminate();

	InnoLowLevelSystem_EXPORT void printLog(const std::string& logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(double logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(const vec2& logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(const vec4& logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(const mat4& logMessage);
	InnoLowLevelSystem_EXPORT void printLogImpl(const std::string& logMessage);

	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};
