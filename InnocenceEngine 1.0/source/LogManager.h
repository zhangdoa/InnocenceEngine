#pragma once
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
private:
	void init() override;
	void update() override;
	void shutdown() override;
};

