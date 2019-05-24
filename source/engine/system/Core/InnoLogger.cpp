#include "InnoLogger.h"
#include <string>
#include "InnoTimer.h"

namespace InnoLoggerNS
{
	std::string GetTimestamp()
	{
		auto l_timeData = InnoTimer::GetCurrentTime(8);
		return
			"["
			+ std::to_string(l_timeData.Year)
			+ "-" + std::to_string(l_timeData.Month)
			+ "-" + std::to_string(l_timeData.Day)
			+ "-" + std::to_string(l_timeData.Hour)
			+ "-" + std::to_string(l_timeData.Minute)
			+ "-" + std::to_string(l_timeData.Second)
			+ "-" + std::to_string(l_timeData.Millisecond)
			+ "]";
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
}

void InnoLogger::Log(LogLevel logLevel, double logMessage)
{
	std::cout << InnoLoggerNS::GetTimestamp() << logMessage << std::endl;
}

void InnoLogger::Log(LogLevel logLevel, const vec2 & logMessage)
{
	std::cout
		<< InnoLoggerNS::GetTimestamp()
		<< "vec2(x: "
		<< logMessage.x
		<< ", y: "
		<< logMessage.y
		<< ")"
		<< std::endl;
}

void InnoLogger::Log(LogLevel logLevel, const vec4 & logMessage)
{
	std::cout
		<< InnoLoggerNS::GetTimestamp()
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

void InnoLogger::Log(LogLevel logLevel, const mat4 & logMessage)
{
	std::cout
		<< InnoLoggerNS::GetTimestamp()
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

void InnoLogger::Log(LogLevel LogLevel, const std::string & logMessage)
{
	std::lock_guard<std::mutex> lock{ InnoLoggerNS::m_Mutex };
#if defined INNO_PLATFORM_WIN
	switch (LogLevel)
	{
	case LogLevel::Verbose:
		std::cout << InnoLoggerNS::blueColor << InnoLoggerNS::GetTimestamp() << logMessage << std::endl; break;
	case LogLevel::Warning:
		std::cout << InnoLoggerNS::yellowColor << InnoLoggerNS::GetTimestamp() << logMessage << std::endl; break;
	case LogLevel::Error:
		std::cout << InnoLoggerNS::redColor << InnoLoggerNS::GetTimestamp() << logMessage << std::endl; break;
	case LogLevel::Success:
		std::cout << InnoLoggerNS::greenColor << InnoLoggerNS::GetTimestamp() << logMessage << std::endl; break;
	default: std::cout << InnoLoggerNS::whiteColor << InnoLoggerNS::GetTimestamp() << logMessage << std::endl; break;
	}
#else
	std::cout << InnoLoggerNS::GetTimestamp() << logMessage << std::endl;
#endif
	InnoLoggerNS::m_LogFile << InnoLoggerNS::GetTimestamp() << logMessage << std::endl;
}

bool InnoLogger::Setup()
{
	InnoLoggerNS::m_LogFile.open(InnoLoggerNS::GetTimestamp() + ".InnoLog", std::ios::out | std::ios::trunc);

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