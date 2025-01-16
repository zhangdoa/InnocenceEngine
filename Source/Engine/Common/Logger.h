#pragma once
#include "STL14.h"

namespace Inno
{
	enum class LogLevel { Verbose, Success, Warning, Error };

	class Logger
	{
	public:
		Logger();
		~Logger();

		template<typename... Args>
		void Log(LogLevel logLevel, Args&&... values)
		{
			if (logLevel < GetDefaultLogLevel())
			{
				return;
			}
			LogStartOfLine(logLevel);
			LogContent(values ...);
			LogEndOfLine();
		}

		void SetDefaultLogLevel(LogLevel logLevel);
		LogLevel GetDefaultLogLevel();

		template<typename Arg>
		void LogContent(Arg&& value)
		{
			LogImpl(value);
		}

		template<typename T, typename... Args>
		void LogContent(T&& first, Args&&... values)
		{
			LogContent(first);
			LogContent(values ...);
		}

	private:
		void LogStartOfLine(LogLevel logLevel);
		void LogEndOfLine();

		void LogImpl(const void* logMessage);
		void LogImpl(bool logMessage);
		void LogImpl(uint8_t logMessage);
		void LogImpl(uint16_t logMessage);
		void LogImpl(uint32_t logMessage);
		void LogImpl(uint64_t logMessage);
		void LogImpl(int8_t logMessage);
		void LogImpl(int16_t logMessage);
		void LogImpl(int32_t logMessage);
		void LogImpl(int64_t logMessage);
		void LogImpl(float logMessage);
		void LogImpl(double logMessage);
		void LogImpl(const char* logMessage);

		std::ofstream m_LogFile;
		std::mutex m_Mutex;
		LogLevel m_LogLevel;
	};
}