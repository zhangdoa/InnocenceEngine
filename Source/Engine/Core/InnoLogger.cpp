#include "InnoLogger.h"
#include <string>
#include "InnoTimer.h"

namespace InnoLoggerNS
{
	inline std::ostream& GetTimestamp(std::ostream &s)
	{
		auto l_timeData = InnoTimer::GetCurrentTime(8);
		s
			<< "["
			<< l_timeData.Year
			<< "-"
			<< l_timeData.Month
			<< "-"
			<< l_timeData.Day
			<< "-"
			<< l_timeData.Hour
			<< "-"
			<< l_timeData.Minute
			<< "-"
			<< l_timeData.Second
			<< "-"
			<< l_timeData.Millisecond
			<< "]";
		return s;
	}

#if defined INNO_PLATFORM_WIN
#include <windows.h>
	inline std::ostream& redColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_RED | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& greenColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& blueColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& yellowColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& whiteColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		return s;
	}
#endif

	std::ofstream m_LogFile;
	std::mutex m_Mutex;
	LogLevel m_LogLevel;
}

void InnoLogger::SetDefaultLogLevel(LogLevel logLevel)
{
	InnoLoggerNS::m_LogLevel = logLevel;
}

LogLevel InnoLogger::GetDefaultLogLevel()
{
	return InnoLoggerNS::m_LogLevel;
}

void InnoLogger::LogStartOfLine(LogLevel logLevel)
{
	InnoLoggerNS::m_Mutex.lock();

#if defined INNO_PLATFORM_WIN
	switch (logLevel)
	{
	case LogLevel::Verbose:
		std::cout << InnoLoggerNS::blueColor; break;
	case LogLevel::Warning:
		std::cout << InnoLoggerNS::yellowColor; break;
	case LogLevel::Error:
		std::cout << InnoLoggerNS::redColor; break;
	case LogLevel::Success:
		std::cout << InnoLoggerNS::greenColor; break;
	default: std::cout << InnoLoggerNS::whiteColor; break;
	}
#endif
	std::cout << InnoLoggerNS::GetTimestamp;
	InnoLoggerNS::m_LogFile << InnoLoggerNS::GetTimestamp;
}

void InnoLogger::LogEndOfLine()
{
	std::cout << std::endl;
	InnoLoggerNS::m_LogFile << std::endl;
	InnoLoggerNS::m_Mutex.unlock();
}

void InnoLogger::LogImpl(const void * logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(bool logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(uint8_t logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(uint16_t logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(uint32_t logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(uint64_t logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(int8_t logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(int16_t logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(int32_t logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(int64_t logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(float logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(double logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

void InnoLogger::LogImpl(const vec2 & logMessage)
{
	std::cout
		<< "vec2(x: "
		<< logMessage.x
		<< ", y: "
		<< logMessage.y
		<< ")";
}

void InnoLogger::LogImpl(const vec4 & logMessage)
{
	std::cout
		<< "vec4(x: "
		<< logMessage.x
		<< ", y: "
		<< logMessage.y
		<< ", z: "
		<< logMessage.z
		<< ", w: "
		<< logMessage.w
		<< ")";
}

void InnoLogger::LogImpl(const mat4 & logMessage)
{
	std::cout
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

void InnoLogger::LogImpl(const char* logMessage)
{
	std::cout << logMessage;
	InnoLoggerNS::m_LogFile << logMessage;
}

bool InnoLogger::Setup()
{
	std::stringstream ss;
	ss << InnoLoggerNS::GetTimestamp << ".InnoLog";
	InnoLoggerNS::m_LogFile.open(ss.str(), std::ios::out | std::ios::trunc);

	if (InnoLoggerNS::m_LogFile.is_open())
	{
		return true;
	}
	else
	{
		Log(LogLevel::Error, "InnoLogger: Can't open log file!");
		return false;
	}
}

bool InnoLogger::Initialize()
{
	Log(LogLevel::Success, "InnoLogger: Initialized.");
	return true;
}

bool InnoLogger::Update()
{
	return true;
}

bool InnoLogger::Terminate()
{
	Log(LogLevel::Success, "InnoLogger: Terminated.");
	InnoLoggerNS::m_LogFile.close();
	return true;
}