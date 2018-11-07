#include "LogSystem.h"
#include <iostream>
#include "../component/LogSystemSingletonComponent.h"
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoLogSystemNS
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::string getLogTimeHeader()
	{
		auto l_timeData = g_pCoreSystem->getTimeSystem()->getCurrentTimeInLocal();
		return 
			"[" 
			+ std::to_string(l_timeData.year)
			+ "-" + std::to_string(l_timeData.month)
			+ "-" + std::to_string(l_timeData.day)
			+ "-" + std::to_string(l_timeData.hour) 
			+ "-" + std::to_string(l_timeData.minute)
			+ "-" + std::to_string(l_timeData.second)
			+ "-" + std::to_string(l_timeData.millisecond)
			+"]";
	}
}

INNO_SYSTEM_EXPORT void InnoLogSystem::printLog(double logMessage)
{
	std::cout << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl;
}

INNO_SYSTEM_EXPORT void InnoLogSystem::printLog(const vec2 & logMessage)
{
	std::cout
		<< InnoLogSystemNS::getLogTimeHeader()
		<<"vec2(x: "
		<< logMessage.x
		<<", y: "
		<< logMessage.y
		<<")"
		<< std::endl;
}

INNO_SYSTEM_EXPORT void InnoLogSystem::printLog(const vec4 & logMessage)
{
	std::cout
		<< InnoLogSystemNS::getLogTimeHeader()
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

INNO_SYSTEM_EXPORT void InnoLogSystem::printLog(const mat4 & logMessage)
{
	std::cout
		<< InnoLogSystemNS::getLogTimeHeader()
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

INNO_SYSTEM_EXPORT void InnoLogSystem::printLog(const std::string& logMessage)
{
	std::cout << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl;
}

INNO_SYSTEM_EXPORT objectStatus InnoLogSystem::getStatus()
{
	return InnoLogSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool InnoLogSystem::setup()
{	
	return true;
}

INNO_SYSTEM_EXPORT bool InnoLogSystem::initialize()
{
	InnoLogSystemNS::m_objectStatus = objectStatus::ALIVE;
	InnoLogSystem::printLog("LogSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoLogSystem::update()
{
	if (LogSystemSingletonComponent::getInstance().m_log.size() > 0)
	{
		std::string l_log;
		if (LogSystemSingletonComponent::getInstance().m_log.tryPop(l_log))
		{
			printLog(l_log);
		}
	}
	return true;
}

INNO_SYSTEM_EXPORT bool InnoLogSystem::terminate()
{
	InnoLogSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("LogSystem has been terminated.");
	return true;
}
