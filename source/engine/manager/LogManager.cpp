#include "LogManager.h"

void LogManager::printLog(double logMessage) const
{
	std::cout << "[" <<g_pTimeManager->getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

void LogManager::printLog(std::string logMessage) const
{
	std::cout << "[" <<g_pTimeManager->getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

void LogManager::printLog(const vec2 & logMessage) const
{
	std::cout
		<< "[" 
		<<g_pTimeManager->getCurrentTimeInLocal()
		<< "]"
		<<"vec2(x: "
		<< logMessage.x
		<<", y: "
		<< logMessage.y
		<<")"
		<< std::endl;
}

void LogManager::printLog(const vec3 & logMessage) const
{
	std::cout
		<< "["
		<<g_pTimeManager->getCurrentTimeInLocal()
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

void LogManager::printLog(const quat & logMessage) const
{
	std::cout
		<< "["
		<<g_pTimeManager->getCurrentTimeInLocal()
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

void LogManager::printLog(const mat4 & logMessage) const
{
	std::cout
		<< "["
		<<g_pTimeManager->getCurrentTimeInLocal()
		<< "]"
		<< std::endl
		<< "|"
		<< logMessage.m[0][0]
		<< "]["
		<< logMessage.m[1][0]
		<< "]["
		<< logMessage.m[2][0]
		<< "]["
		<< logMessage.m[3][0]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m[0][1]
		<< "]["
		<< logMessage.m[1][1]
		<< "]["
		<< logMessage.m[2][1]
		<< "]["
		<< logMessage.m[3][1]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m[0][2]
		<< "]["
		<< logMessage.m[1][2]
		<< "]["
		<< logMessage.m[2][2]
		<< "]["
		<< logMessage.m[3][2]
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m[0][3]
		<< "]["
		<< logMessage.m[1][3]
		<< "]["
		<< logMessage.m[2][3]
		<< "]["
		<< logMessage.m[3][3]
		<< "|"
		<< std::endl;
}
void LogManager::printLog(const std::thread::id logMessage) const
{
	std::cout << "[" <<g_pTimeManager->getCurrentTimeInLocal() << "]"<< "Thread: " << logMessage << std::endl;
}

const objectStatus & LogManager::getStatus() const
{
	return m_objectStatus;
}

void LogManager::setStatus(objectStatus objectStatus)
{
	m_objectStatus = objectStatus;
}

void LogManager::setup()
{	
}

void LogManager::initialize()
{
	this->setStatus(objectStatus::ALIVE);
	printLog("LogManager has been initialized.");
}

void LogManager::update()
{
}

void LogManager::shutdown()
{
	this->setStatus(objectStatus::SHUTDOWN);
	printLog("LogManager has been shutdown.");
}
