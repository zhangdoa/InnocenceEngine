#include "../../main/stdafx.h"
#include "LogManager.h"
#include "TimeManager.h"


LogManager::LogManager()
{
}


LogManager::~LogManager()
{
}

void LogManager::printLog(float logMessage)
{
	std::cout << "[" << TimeManager::getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

void LogManager::printLog(std::string logMessage)
{
	std::cout << "[" << TimeManager::getCurrentTimeInLocal()  << "]" << logMessage << std::endl;
}

void LogManager::printLog(const glm::vec2 & logMessage)
{
	std::cout
		<< "[" 
		<< TimeManager::getCurrentTimeInLocal() 
		<< "]"
		<<"glm::vec2(x: "
		<< logMessage.x
		<<", y: "
		<< logMessage.y
		<<")"
		<< std::endl;
}

void LogManager::printLog(const glm::vec3 & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getCurrentTimeInLocal()
		<< "]"
		<< "glm::vec3(x: "
		<< logMessage.x
		<< ", y: "
		<< logMessage.y
		<< ", z: "
		<< logMessage.z
		<< ")"
		<< std::endl;
}

void LogManager::printLog(const glm::quat & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getCurrentTimeInLocal()
		<< "]"
		<< "glm::quat(x: "
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

void LogManager::printLog(const glm::mat4 & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getCurrentTimeInLocal()
		<< "]"
		<< std::endl
		<< "|"
		<< logMessage[0][0]
		<< "]["
		<< logMessage[0][1]
		<< "]["
		<< logMessage[0][2]
		<< "]["
		<< logMessage[0][3]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage[1][0]
		<< "]["
		<< logMessage[1][1]
		<< "]["
		<< logMessage[1][2]
		<< "]["
		<< logMessage[1][3]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage[2][0]
		<< "]["
		<< logMessage[2][1]
		<< "]["
		<< logMessage[2][2]
		<< "]["
		<< logMessage[2][3]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage[3][0]
		<< "]["
		<< logMessage[3][1]
		<< "]["
		<< logMessage[3][2]
		<< "]["
		<< logMessage[3][3]
		<< "|"
		<< std::endl;
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
