#pragma once
#include "BaseManager.h"
#include "entity/InnoMath.h"

class LogManager : public BaseManager
{
public:
	LogManager() {};
	~LogManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void printLog(double logMessage);
	void printLog(std::string logMessage);
	void printLog(const vec2& logMessage);
	void printLog(const vec3& logMessage);
	void printLog(const quat& logMessage);
	void printLog(const mat4& logMessage);
	void printLog(const std::thread::id logMessage);
};

