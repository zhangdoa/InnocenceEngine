#pragma once
#include "../interface/IManager.h"

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
	void printLog(const glm::vec2& logMessage);
	void printLog(const glm::vec3& logMessage);
	void printLog(const glm::quat& logMessage);
	void printLog(const glm::mat4& logMessage);

	void printLog(const std::thread::id logMessage);

private:
	LogManager() {};
};

