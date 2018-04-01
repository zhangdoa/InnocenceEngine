#include "LogSystem.h"

void LogSystem::printLog(double logMessage) const
{
	std::cout << "[" <<g_pTimeSystem->getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

void LogSystem::printLog(std::string logMessage) const
{
	std::cout << "[" <<g_pTimeSystem->getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

void LogSystem::printLog(const vec2 & logMessage) const
{
	std::cout
		<< "[" 
		<<g_pTimeSystem->getCurrentTimeInLocal()
		<< "]"
		<<"vec2(x: "
		<< logMessage.x
		<<", y: "
		<< logMessage.y
		<<")"
		<< std::endl;
}

void LogSystem::printLog(const vec4 & logMessage) const
{
	std::cout
		<< "["
		<<g_pTimeSystem->getCurrentTimeInLocal()
		<< "]"
		<< "vec4(x: "
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

void LogSystem::printLog(const mat4 & logMessage) const
{
	std::cout
		<< "["
		<<g_pTimeSystem->getCurrentTimeInLocal()
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

const objectStatus & LogSystem::getStatus() const
{
	return m_objectStatus;
}

void LogSystem::setup()
{	
}

void LogSystem::initialize()
{
	m_objectStatus = objectStatus::ALIVE;
	printLog("LogSystem has been initialized.");
}

void LogSystem::update()
{
}

void LogSystem::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
	printLog("LogSystem has been shutdown.");
}
