#include "Logger.h"

#include <string>

#include "Timer.h"

using namespace Inno;
namespace Inno
{
	namespace LoggerNS
	{
		inline std::ostream& GetTimestamp(std::ostream& s)
		{
			auto l_timeData = Timer::GetCurrentTime();
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
		inline std::ostream& redColor(std::ostream& s)
		{
			HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(hStdout,
				FOREGROUND_RED | FOREGROUND_INTENSITY);
			return s;
		}
		inline std::ostream& greenColor(std::ostream& s)
		{
			HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(hStdout,
				FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			return s;
		}
		inline std::ostream& blueColor(std::ostream& s)
		{
			HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(hStdout,
				FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			return s;
		}
		inline std::ostream& yellowColor(std::ostream& s)
		{
			HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(hStdout,
				FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
			return s;
		}
		inline std::ostream& whiteColor(std::ostream& s)
		{
			HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(hStdout,
				FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			return s;
		}
#endif
	}
}

void Logger::SetDefaultLogLevel(LogLevel logLevel)
{
	m_LogLevel = logLevel;
}

LogLevel Logger::GetDefaultLogLevel()
{
	return m_LogLevel;
}

void Logger::LogStartOfLine(LogLevel logLevel)
{
	m_Mutex.lock();

#if defined INNO_PLATFORM_WIN
	switch (logLevel)
	{
	case LogLevel::Verbose:
		std::cout << LoggerNS::blueColor; break;
	case LogLevel::Warning:
		std::cout << LoggerNS::yellowColor; break;
	case LogLevel::Error:
		std::cout << LoggerNS::redColor; break;
	case LogLevel::Success:
		std::cout << LoggerNS::greenColor; break;
	default: std::cout << LoggerNS::whiteColor; break;
	}
#endif
	std::cout << LoggerNS::GetTimestamp;
	m_LogFile << LoggerNS::GetTimestamp;
}

void Logger::LogEndOfLine()
{
	std::cout << std::endl;
	m_LogFile << std::endl;
	m_Mutex.unlock();
}

void Logger::LogImpl(const void* logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(bool logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(uint8_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(uint16_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(uint32_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(uint64_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(int8_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(int16_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(int32_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(int64_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(float logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(double logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void Logger::LogImpl(const char* logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

Logger::Logger()
{
	std::stringstream ss;
	ss << LoggerNS::GetTimestamp << ".Log";
	m_LogFile.open(ss.str(), std::ios::out | std::ios::trunc);

	if (!m_LogFile.is_open())
	{
		Log(LogLevel::Error, "Logger: Can't open log file!");
	}
}

Logger::~Logger()
{
	Log(LogLevel::Success, "Logger: Terminated.");
	m_LogFile.close();
}