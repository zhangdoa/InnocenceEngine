#pragma once
#include "IEventManager.h"
class LogManager : public IEventManager
{
public:
	LogManager();
	~LogManager();
	static void LogManager::printLog(std::string logMessage);
private:
	void init() override;
	void update() override;
	void shutdown() override;
};

