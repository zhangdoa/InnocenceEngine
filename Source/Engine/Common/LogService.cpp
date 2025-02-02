#include "LogService.h"
#include "Config.h"

#include <string>

#include "Timer.h"
#include "../Engine.h"

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

		std::string ApplyColor(LogLevel logLevel) 
		{
			switch (logLevel) 
			{
				case LogLevel::Verbose:
					return "\033[34m";  // Blue
				case LogLevel::Warning:
					return "\033[33m";  // Yellow
				case LogLevel::Error:
					return "\033[31m";  // Red
				case LogLevel::Success:
					return "\033[32m";  // Green
				default:
					return "\033[0m";   // Reset (White)
			}
		}

		std::string LogLevelToString(LogLevel logLevel) 
		{
			switch (logLevel) 
			{
				case LogLevel::Verbose: return "Verbose";
				case LogLevel::Success: return "Success";
				case LogLevel::Warning: return "Warning";
				case LogLevel::Error: return "Error";
				default: return "UNKNOWN";
			}
		}

		// Function to check if the output stream is a console
		bool IsConsole(std::ostream& os) 
		{
			return &os == &std::cout || &os == &std::cerr;
		}
		
		std::string ApplyColor(const char* context) 
		{
			// Generate a hue value based on the hash of the context
			size_t hash = std::hash<std::string>()(context);
			int hue = hash % 360; // Hue value in the range [0, 360)

			// Convert hue to RGB
			float r, g, b;
			int i = hue / 60;
			float f = hue / 60.0f - i;
			float q = 1 - f;

			switch (i) {
				case 0: r = 1; g = f; b = 0; break;
				case 1: r = q; g = 1; b = 0; break;
				case 2: r = 0; g = 1; b = f; break;
				case 3: r = 0; g = q; b = 1; break;
				case 4: r = f; g = 0; b = 1; break;
				case 5: r = 1; g = 0; b = q; break;
			}

			// Convert RGB to 256-color ANSI code
			int colorCode = 16 + (int)(r * 5) * 36 + (int)(g * 5) * 6 + (int)(b * 5);

			if (IsConsole(std::cout)) 
			{
				return "\033[38;5;" + std::to_string(colorCode) + "m"; // 256-color ANSI escape
			} else {
				return ""; // No color for non-console environments
			}
		}

		std::string ResetColor() 
		{
			return "\033[0m"; // ANSI reset
		}
	}
}

void LogService::SetDefaultLogLevel(LogLevel logLevel)
{
	m_LogLevel = logLevel;
}

LogLevel LogService::GetDefaultLogLevel()
{
	return m_LogLevel;
}

void LogService::LogStartOfLine(LogLevel logLevel, const char* context)
{
	m_Mutex.lock();

	std::cout << LoggerNS::GetTimestamp;
	std::cout << LoggerNS::ApplyColor(logLevel);
	std::cout << "[" << LoggerNS::LogLevelToString(logLevel) << "]";
	std::cout << LoggerNS::ApplyColor(context);
	std::cout << "[" << context << "] ";
	std::cout << LoggerNS::ResetColor();
	
	m_LogFile << LoggerNS::GetTimestamp;
	m_LogFile << "[" << LoggerNS::LogLevelToString(logLevel) << "]";
	m_LogFile << "[" << context << "] ";
}

void LogService::LogEndOfLine()
{
	std::cout << std::endl;
	std::cout << LoggerNS::ResetColor();
	
	m_LogFile << std::endl;
	
	m_Mutex.unlock();
}

void LogService::LogImpl(const void* logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(bool logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(uint8_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(uint16_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(uint32_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(uint64_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(int8_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(int16_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(int32_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(int64_t logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(float logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(double logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(const char* logMessage)
{
	std::cout << logMessage;
	m_LogFile << logMessage;
}

void LogService::LogImpl(const wchar_t* logMessage)
{
	std::wcout << logMessage;
	m_LogFile << logMessage;
}

LogService::LogService()
{
	std::stringstream ss;
	ss << LoggerNS::GetTimestamp << ".Log";
	m_LogFile.open(ss.str(), std::ios::out | std::ios::trunc);

	if (!m_LogFile.is_open())
	{
		Log(Error, "Can't open log file!");
	}
}

LogService::~LogService()
{
	Log(Success, "Terminated.");
	m_LogFile.close();
}