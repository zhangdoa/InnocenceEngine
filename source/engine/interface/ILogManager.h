#pragma once
#include "common/stdafx.h"
#include "IManager.h"
#include "entity/InnoMath.h"

class ILogManager : public IManager
{
public:
	virtual ~ILogManager() {};
	virtual void printLog(double logMessage) const = 0;
	virtual void printLog(std::string logMessage) const = 0;
	virtual void printLog(const vec2& logMessage) const = 0;
	virtual void printLog(const vec3& logMessage) const = 0;
	virtual void printLog(const quat& logMessage) const = 0;
	virtual void printLog(const mat4& logMessage) const = 0;
	virtual void printLog(const std::thread::id logMessage) const = 0;
};

ILogManager* g_pLogManager;