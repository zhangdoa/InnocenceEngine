#include "LogSystem.h"
#include <iostream>
#include "TimeSystem.h"
#include "../../component/LogSystemSingletonComponent.h"

namespace InnoLogSystem
{
	objectStatus m_LogSystemStatus = objectStatus::SHUTDOWN;
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLog(double logMessage)
{
	std::cout << "[" <<InnoTimeSystem::getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLog(const std::string& logMessage)
{
	std::cout << "[" <<InnoTimeSystem::getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLog(const vec2 & logMessage)
{
	std::cout
		<< "[" 
		<<InnoTimeSystem::getCurrentTimeInLocal()
		<< "]"
		<<"vec2(x: "
		<< logMessage.x
		<<", y: "
		<< logMessage.y
		<<")"
		<< std::endl;
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLog(const vec4 & logMessage)
{
	std::cout
		<< "["
		<<InnoTimeSystem::getCurrentTimeInLocal()
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

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLog(const mat4 & logMessage)
{
	std::cout
		<< "["
		<<InnoTimeSystem::getCurrentTimeInLocal()
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

InnoLowLevelSystem_EXPORT objectStatus InnoLogSystem::getStatus()
{
	return m_LogSystemStatus;
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::setup()
{	
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::initialize()
{
	m_LogSystemStatus = objectStatus::ALIVE;
	LogSystemSingletonComponent::getInstance().m_log.push("LogSystem has been initialized.");
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::update()
{
	if (LogSystemSingletonComponent::getInstance().m_log.size() > 0)
	{
		std::string l_log;
		if (LogSystemSingletonComponent::getInstance().m_log.tryPop(l_log))
		{
			printLog(l_log);
		}
	}
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::shutdown()
{
	m_LogSystemStatus = objectStatus::SHUTDOWN;
	LogSystemSingletonComponent::getInstance().m_log.push("LogSystem has been shutdown.");
}
