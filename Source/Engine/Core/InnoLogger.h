#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoMathHelper.h"

class InnoLogger
{
public:
	static bool Setup();
	static bool Initialize();
	static bool Update();
	static bool Terminate();

	template<typename... Args>
	static void Log(LogLevel logLevel, Args&&... values)
	{
		if (logLevel < GetDefaultLogLevel())
		{
			return;
		}
		LogStartOfLine(logLevel);
		LogContent(values ...);
		LogEndOfLine();
	}

	static void SetDefaultLogLevel(LogLevel logLevel);
	static LogLevel GetDefaultLogLevel();

	static void LogStartOfLine(LogLevel logLevel);

	template<typename Arg>
	static void LogContent(Arg&& value)
	{
		LogImpl(value);
	}

	template<typename T, typename... Args>
	static void LogContent(T&& first, Args&&... values)
	{
		LogContent(first);
		LogContent(values ...);
	}

	static void LogEndOfLine();

	static void LogImpl(const void* logMessage);
	static void LogImpl(bool logMessage);
	static void LogImpl(uint8_t logMessage);
	static void LogImpl(uint16_t logMessage);
	static void LogImpl(uint32_t logMessage);
	static void LogImpl(uint64_t logMessage);
	static void LogImpl(int8_t logMessage);
	static void LogImpl(int16_t logMessage);
	static void LogImpl(int32_t logMessage);
	static void LogImpl(int64_t logMessage);
	static void LogImpl(float logMessage);
	static void LogImpl(double logMessage);
	static void LogImpl(const Vec2& logMessage);
	static void LogImpl(const Vec4& logMessage);
	static void LogImpl(const Mat4& logMessage);
	static void LogImpl(const char* logMessage);
};
