#include "LogSystem.h"
#include "../Core/Logger.h"

using namespace Inno;
namespace Inno
{
	namespace LogSystemNS
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	}
}

ObjectStatus LogSystem::GetStatus()
{
	return LogSystemNS::m_ObjectStatus;
}

void LogSystem::SetDefaultLogLevel(LogLevel logLevel)
{
	Logger::SetDefaultLogLevel(logLevel);
}

LogLevel LogSystem::GetDefaultLogLevel()
{
	return Logger::GetDefaultLogLevel();
}

void LogSystem::LogStartOfLine(LogLevel logLevel)
{
	Logger::LogStartOfLine(logLevel);
}

void LogSystem::LogEndOfLine()
{
	Logger::LogEndOfLine();
}

void LogSystem::LogImpl(const void* logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(bool logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(uint8_t logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(uint16_t logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(uint32_t logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(uint64_t logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(int8_t logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(int16_t logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(int32_t logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(int64_t logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(float logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(double logMessage)
{
	Logger::LogImpl(logMessage);
}

void LogSystem::LogImpl(const char* logMessage)
{
	Logger::LogImpl(logMessage);
}

bool LogSystem::Setup(ISystemConfig* systemConfig)
{
	LogSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return Logger::Setup();
}

bool LogSystem::Initialize()
{
	if (LogSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		LogSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		return Logger::Initialize();
	}
	else
	{
		return false;
	}
}

bool LogSystem::Update()
{
	if (LogSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return 	Logger::Update();
	}
	else
	{
		LogSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool LogSystem::Terminate()
{
	LogSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	return Logger::Terminate();
}