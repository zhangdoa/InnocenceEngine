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
	void printLog(float logMessage);
	void printLog(std::string logMessage);
	void printLog(const glm::vec2& logMessage);
	void printLog(const glm::vec3& logMessage);
	void printLog(const glm::quat& logMessage);
	void printLog(const glm::mat4& logMessage);
private:
	void init() override;
	void update() override;
	void shutdown() override;
};

