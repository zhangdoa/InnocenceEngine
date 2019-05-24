#pragma once
#include "../../common/InnoMath.h"

enum class LogLevel { Verbose, Success, Warning, Error };

namespace InnoLogger
{
	bool Setup();
	bool Initialize();
	bool Update();
	bool Terminate();

	void Log(LogLevel logLevel, double logMessage);
	void Log(LogLevel logLevel, const vec2& logMessage);
	void Log(LogLevel logLevel, const vec4& logMessage);
	void Log(LogLevel logLevel, const mat4& logMessage);
	void Log(LogLevel logLevel, const std::string& logMessage);
};
