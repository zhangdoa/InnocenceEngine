#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "entity/InnoMath.h"

class ILogSystem : public ISystem
{
public:
	virtual ~ILogSystem() {};
	virtual void printLog(double logMessage) const = 0;
	virtual void printLog(std::string logMessage) const = 0;
	virtual void printLog(const vec2& logMessage) const = 0;
	virtual void printLog(const vec4& logMessage) const = 0;
	virtual void printLog(const mat4& logMessage) const = 0;
};