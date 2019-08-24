#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoMathHelper.h"
#include "../Common/InnoClassTemplate.h"

class ILogSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ILogSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

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

	virtual void SetDefaultLogLevel(LogLevel logLevel) = 0;
	virtual LogLevel GetDefaultLogLevel() = 0;

protected:
	virtual void LogStartOfLine(LogLevel logLevel) = 0;

	virtual void LogEndOfLine() = 0;

	virtual void LogImpl(const void* logMessage) = 0;
	virtual void LogImpl(bool logMessage) = 0;
	virtual void LogImpl(uint8_t logMessage) = 0;
	virtual void LogImpl(uint16_t logMessage) = 0;
	virtual void LogImpl(uint32_t logMessage) = 0;
	virtual void LogImpl(uint64_t logMessage) = 0;
	virtual void LogImpl(int8_t logMessage) = 0;
	virtual void LogImpl(int16_t logMessage) = 0;
	virtual void LogImpl(int32_t logMessage) = 0;
	virtual void LogImpl(int64_t logMessage) = 0;
	virtual void LogImpl(float logMessage) = 0;
	virtual void LogImpl(double logMessage) = 0;
	virtual void LogImpl(const Vec2& logMessage) = 0;
	virtual void LogImpl(const Vec4& logMessage) = 0;
	virtual void LogImpl(const Mat4& logMessage) = 0;
	virtual void LogImpl(const char* logMessage) = 0;
};
