#pragma once
#include "ISystem.h"
#include "../common/InnoMath.h"

class ILogSystem : public ISystem
{
public:
	virtual ~ILogSystem() {};
	virtual void printLog(double logMessage) const = 0;
	virtual void printLog(const std::string& logMessage) const = 0;
	virtual void printLog(const vec2<double>& logMessage) const = 0;
	virtual void printLog(const vec4<double>& logMessage) const = 0;
	virtual void printLog(const mat4<double>& logMessage) const = 0;
};