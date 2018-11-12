#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

INNO_INTERFACE ILogSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ILogSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual void printLog(double logMessage) = 0;
	INNO_SYSTEM_EXPORT virtual void printLog(const vec2& logMessage) = 0;
	INNO_SYSTEM_EXPORT virtual void printLog(const vec4& logMessage) = 0;
	INNO_SYSTEM_EXPORT virtual void printLog(const mat4& logMessage) = 0;
	INNO_SYSTEM_EXPORT virtual void printLog(const std::string& logMessage) = 0;

	INNO_SYSTEM_EXPORT virtual objectStatus getStatus() = 0;
};
