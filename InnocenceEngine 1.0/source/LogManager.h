#pragma once
#include "Math.h"
#include "IEventManager.h"
class LogManager : public IEventManager
{
public:
	LogManager();
	~LogManager();

	static LogManager& getInstance()
	{
		static LogManager instance;
		return instance;
	}

	static void LogManager::printLog(std::string logMessage);
	static void LogManager::printLog(const Vec2f& logMessage);
	static void LogManager::printLog(const Vec3f& logMessage);
	static void LogManager::printLog(const Vec4f& logMessage);
	static void LogManager::printLog(const Mat4f& logMessage);
private:
	void init() override;
	void update() override;
	void shutdown() override;
};

