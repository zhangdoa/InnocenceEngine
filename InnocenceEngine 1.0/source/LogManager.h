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
	static void LogManager::printLog(float logMessage);
	static void LogManager::printLog(std::string logMessage);
	static void LogManager::printLog(const glm::vec2& logMessage);
	static void LogManager::printLog(const glm::vec3& logMessage);
	static void LogManager::printLog(const glm::quat& logMessage);
	static void LogManager::printLog(const glm::mat4& logMessage);
private:
	void init() override;
	void update() override;
	void shutdown() override;
};

