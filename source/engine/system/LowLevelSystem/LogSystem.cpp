#include "LogSystem.h"
#include <iostream>
#include "TimeSystem.h"
#include "../../component/LogSystemSingletonComponent.h"

namespace InnoLogSystem
{
	objectStatus m_LogSystemStatus = objectStatus::SHUTDOWN;
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLog(const std::string& logMessage)
{
	// @TODO: precisely print log, generic
	LogSystemSingletonComponent::getInstance().m_log.push(logMessage);
	//printLogImpl(logMessage);
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLogImpl(double logMessage)
{
	std::cout << "[" <<InnoTimeSystem::getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLogImpl(const vec2 & logMessage)
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

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLogImpl(const vec4 & logMessage)
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

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLogImpl(const mat4 & logMessage)
{
	std::cout
		<< "["
		<<InnoTimeSystem::getCurrentTimeInLocal()
		<< "]"
		<< std::endl
		<< "|"
		<< logMessage.m00
		<< ""
		<< logMessage.m10
		<< ""
		<< logMessage.m20
		<< ""
		<< logMessage.m30
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m01
		<< ""
		<< logMessage.m11
		<< ""
		<< logMessage.m21
		<< ""
		<< logMessage.m31
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m02
		<< ""
		<< logMessage.m12
		<< ""
		<< logMessage.m22
		<< ""
		<< logMessage.m32
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m03
		<< ""
		<< logMessage.m13
		<< ""
		<< logMessage.m23
		<< ""
		<< logMessage.m33
		<< "|"
		<< std::endl;
}

InnoLowLevelSystem_EXPORT void InnoLogSystem::printLogImpl(const std::string& logMessage)
{
	std::cout << "[" << InnoTimeSystem::getCurrentTimeInLocal() << "]" << logMessage << std::endl;
}

InnoLowLevelSystem_EXPORT objectStatus InnoLogSystem::getStatus()
{
	return m_LogSystemStatus;
}

InnoLowLevelSystem_EXPORT bool InnoLogSystem::setup()
{	
	return true;
}

InnoLowLevelSystem_EXPORT bool InnoLogSystem::initialize()
{
	m_LogSystemStatus = objectStatus::ALIVE;
	InnoLogSystem::printLog("LogSystem has been initialized.");
	return true;
}

InnoLowLevelSystem_EXPORT bool InnoLogSystem::update()
{
	if (LogSystemSingletonComponent::getInstance().m_log.size() > 0)
	{
		std::string l_log;
		if (LogSystemSingletonComponent::getInstance().m_log.tryPop(l_log))
		{
			printLogImpl(l_log);
		}
	}
	return true;
}

InnoLowLevelSystem_EXPORT bool InnoLogSystem::terminate()
{
	m_LogSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("LogSystem has been terminated.");
	return true;
}
