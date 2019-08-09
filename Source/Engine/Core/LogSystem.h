#pragma once
#include "ILogSystem.h"

class InnoLogSystem : public ILogSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoLogSystem);

	// Inherited via ILogSystem
	virtual bool setup() override;
	virtual bool initialize() override;
	virtual bool update() override;
	virtual bool terminate() override;

	virtual ObjectStatus getStatus() override;

	virtual void SetDefaultLogLevel(LogLevel logLevel) override;

	virtual LogLevel GetDefaultLogLevel() override;

	virtual void LogStartOfLine(LogLevel logLevel) override;

	virtual void LogEndOfLine() override;

	virtual void LogImpl(const void * logMessage) override;
	virtual void LogImpl(bool logMessage) override;
	virtual void LogImpl(uint8_t logMessage) override;
	virtual void LogImpl(uint16_t logMessage) override;
	virtual void LogImpl(uint32_t logMessage) override;
	virtual void LogImpl(uint64_t logMessage) override;
	virtual void LogImpl(int8_t logMessage) override;
	virtual void LogImpl(int16_t logMessage) override;
	virtual void LogImpl(int32_t logMessage) override;
	virtual void LogImpl(int64_t logMessage) override;
	virtual void LogImpl(float logMessage) override;
	virtual void LogImpl(double logMessage) override;
	virtual void LogImpl(const vec2 & logMessage) override;
	virtual void LogImpl(const vec4 & logMessage) override;
	virtual void LogImpl(const mat4 & logMessage) override;
	virtual void LogImpl(const char * logMessage) override;
};
