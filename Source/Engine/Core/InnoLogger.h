#pragma once
#include "../Common/InnoMath.h"

enum class LogLevel { Verbose, Success, Warning, Error };

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
		LogStartOfLine(logLevel);
		LogContent(values ...);
		LogEndOfLine();
	}

private:
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
	static void LogImpl(const vec2& logMessage);
	static void LogImpl(const vec4& logMessage);
	static void LogImpl(const mat4& logMessage);
	static void LogImpl(const char* logMessage);
};
