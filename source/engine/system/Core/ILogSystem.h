#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoMath.h"
#include "../../common/InnoClassTemplate.h"

INNO_INTERFACE ILogSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ILogSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual void printLog(double logMessage) = 0;
	virtual void printLog(const vec2& logMessage) = 0;
	virtual void printLog(const vec4& logMessage) = 0;
	virtual void printLog(const mat4& logMessage) = 0;
	virtual void printLog(LogType LogType, const std::string& logMessage) = 0;

	virtual ObjectStatus getStatus() = 0;
};
