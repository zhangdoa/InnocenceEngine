#include "../../main/stdafx.h"
#include "LogManager.h"
#include "TimeManager.h"

void LogManager::printLog(float logMessage)
{
	std::cout << "[" << TimeManager::getInstance().getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

void LogManager::printLog(std::string logMessage)
{
	std::cout << "[" << TimeManager::getInstance().getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

void LogManager::printLog(const vec2 & logMessage)
{
	std::cout
		<< "[" 
		<< TimeManager::getInstance().getCurrentTimeInLocal()
		<< "]"
		<<"vec2(x: "
		<< logMessage.x
		<<", y: "
		<< logMessage.y
		<<")"
		<< std::endl;
}

void LogManager::printLog(const vec3 & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getInstance().getCurrentTimeInLocal()
		<< "]"
		<< "vec3(x: "
		<< logMessage.x
		<< ", y: "
		<< logMessage.y
		<< ", z: "
		<< logMessage.z
		<< ")"
		<< std::endl;
}

void LogManager::printLog(const quat & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getInstance().getCurrentTimeInLocal()
		<< "]"
		<< "quat(x: "
		<< logMessage.x
		<< ", y: "
		<< logMessage.y
		<< ", z: "
		<< logMessage.z
		<< ", w: "
		<< logMessage.w
		<< ")"
		<< std::endl;
}

void LogManager::printLog(const mat4 & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getInstance().getCurrentTimeInLocal()
		<< "]"
		<< std::endl
		<< "|"
		<< logMessage.m[0][0]
		<< "]["
		<< logMessage.m[0][1]
		<< "]["
		<< logMessage.m[0][2]
		<< "]["
		<< logMessage.m[0][3]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m[1][0]
		<< "]["
		<< logMessage.m[1][1]
		<< "]["
		<< logMessage.m[1][2]
		<< "]["
		<< logMessage.m[1][3]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m[2][0]
		<< "]["
		<< logMessage.m[2][1]
		<< "]["
		<< logMessage.m[2][2]
		<< "]["
		<< logMessage.m[2][3]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m[3][0]
		<< "]["
		<< logMessage.m[3][1]
		<< "]["
		<< logMessage.m[3][2]
		<< "]["
		<< logMessage.m[3][3]
		<< "|"
		<< std::endl;
}

void LogManager::printLog(const std::thread::id logMessage)
{
	std::cout << "[" << TimeManager::getInstance().getCurrentTimeInLocal() << "]"<< "Thread: " << logMessage << std::endl;
}

void LogManager::setup()
{
}

void LogManager::initialize()
{
}

void LogManager::update()
{
}

void LogManager::shutdown()
{
}
