#pragma once
#include "ISystem.h"

class ILogSystem : public ISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ILogSystem);

	template<typename... Args>
	inline void Log(LogLevel logLevel, Args&&... values)
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
	inline void LogContent(Arg&& value)
	{
		LogImpl(value);
	}

	template<typename T, typename... Args>
	inline void LogContent(T&& first, Args&&... values)
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
	virtual void LogImpl(const char* logMessage) = 0;
};
