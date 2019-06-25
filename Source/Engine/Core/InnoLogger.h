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

	static void LogImpl(double logMessage);
	static void LogImpl(const vec2& logMessage);
	static void LogImpl(const vec4& logMessage);
	static void LogImpl(const mat4& logMessage);
	static void LogImpl(const char* logMessage);
};
