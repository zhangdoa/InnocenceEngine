#include "LogSystem.h"
#include "InnoLogger.h"

namespace InnoLogSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

ObjectStatus InnoLogSystem::getStatus()
{
	return InnoLogSystemNS::m_objectStatus;
}

void InnoLogSystem::SetDefaultLogLevel(LogLevel logLevel)
{
	InnoLogger::SetDefaultLogLevel(logLevel);
}

LogLevel InnoLogSystem::GetDefaultLogLevel()
{
	return InnoLogger::GetDefaultLogLevel();
}

void InnoLogSystem::LogStartOfLine(LogLevel logLevel)
{
	InnoLogger::LogStartOfLine(logLevel);
}

void InnoLogSystem::LogEndOfLine()
{
	InnoLogger::LogEndOfLine();
}

void InnoLogSystem::LogImpl(const void * logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(bool logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(uint8_t logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(uint16_t logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(uint32_t logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(uint64_t logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(int8_t logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(int16_t logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(int32_t logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(int64_t logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(float logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(double logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(const vec2 & logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(const vec4 & logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(const mat4 & logMessage)
{
	InnoLogger::LogImpl(logMessage);
}

void InnoLogSystem::LogImpl(const char * logMessage)
{
	InnoLogger::LogImpl(logMessage);
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