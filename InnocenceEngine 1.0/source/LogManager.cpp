#include "stdafx.h"
#include "LogManager.h"
#include "TimeManager.h"


LogManager::LogManager()
{
}


LogManager::~LogManager()
{
}

void LogManager::printLog(std::string logMessage)
{
	std::cout << "[" << TimeManager::getCurrentTimeInLocal()  << "]" << logMessage << std::endl;
}

void LogManager::init()
{
}

void LogManager::update()
{
}

void LogManager::shutdown()
{
}
