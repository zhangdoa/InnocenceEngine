#include "LogSystem.h"
#include "InnoLogger.h"

INNO_PRIVATE_SCOPE InnoLogSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

void InnoLogSystem::printLog(double logMessage)
{
	InnoLogger::Log(LogLevel::Verbose, logMessage);
}

void InnoLogSystem::printLog(const vec2 & logMessage)
{
	InnoLogger::Log(LogLevel::Verbose, logMessage);
}

void InnoLogSystem::printLog(const vec4 & logMessage)
{
	InnoLogger::Log(LogLevel::Verbose, logMessage);
}

void InnoLogSystem::printLog(const mat4 & logMessage)
{
	InnoLogger::Log(LogLevel::Verbose, logMessage);
}

void InnoLogSystem::printLog(LogType LogType, const std::string & logMessage)
{
	switch (LogType)
	{
	case LogType::INNO_DEV_VERBOSE:
		InnoLogger::Log(LogLevel::Verbose, logMessage.c_str());
		break;
	case LogType::INNO_WARNING:
		InnoLogger::Log(LogLevel::Warning, logMessage.c_str());
		break;
	case LogType::INNO_ERROR:
		InnoLogger::Log(LogLevel::Error, logMessage.c_str());
		break;
	case LogType::INNO_DEV_SUCCESS:
		InnoLogger::Log(LogLevel::Success, logMessage.c_str());
		break;
	default:
		break;
	}
}

ObjectStatus InnoLogSystem::getStatus()
{
	return InnoLogSystemNS::m_objectStatus;
}

bool InnoLogSystem::setup()
{
	InnoLogSystemNS::m_objectStatus = ObjectStatus::Created;
	return InnoLogger::Setup();
}

bool InnoLogSystem::initialize()
{
	if (InnoLogSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoLogSystemNS::m_objectStatus = ObjectStatus::Activated;
		return InnoLogger::Initialize();
	}
	else
	{
		return false;
	}
}

bool InnoLogSystem::update()
{
	if (InnoLogSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		return 	InnoLogger::Update();
	}
	else
	{
		InnoLogSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoLogSystem::terminate()
{
	InnoLogSystemNS::m_objectStatus = ObjectStatus::Terminated;
	return InnoLogger::Terminate();
}