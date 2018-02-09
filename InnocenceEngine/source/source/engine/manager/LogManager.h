#pragma once
#include "../interface/IManager.h"
#include "../data/InnoMath.h"
class LogManager : public IManager
{
public:
	~LogManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static LogManager& getInstance()
	{
		static LogManager instance;
		return instance;
	}

	void printLog(float logMessage);
	void printLog(std::string logMessage);
	void printLog(const vec2& logMessage);
	void printLog(const vec3& logMessage);
	void printLog(const quat& logMessage);
	void printLog(const mat4& logMessage);
	void printLog(const std::thread::id logMessage);

private:
	LogManager() {};
};

