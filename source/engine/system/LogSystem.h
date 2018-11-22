#pragma once
#include "ILogSystem.h"

class InnoLogSystem : INNO_IMPLEMENT ILogSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoLogSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT void printLog(double logMessage) override;
	INNO_SYSTEM_EXPORT void printLog(const vec2& logMessage) override;
	INNO_SYSTEM_EXPORT void printLog(const vec4& logMessage) override;
	INNO_SYSTEM_EXPORT void printLog(const mat4& logMessage) override;
	INNO_SYSTEM_EXPORT void printLog(logType logType, const std::string& logMessage) override;

	INNO_SYSTEM_EXPORT objectStatus getStatus();
};
