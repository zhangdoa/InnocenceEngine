#pragma once
#include "../Interface/ILogSystem.h"

class InnoLogSystem : public ILogSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoLogSystem);

	// Inherited via ILogSystem
	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void SetDefaultLogLevel(LogLevel logLevel) override;

	LogLevel GetDefaultLogLevel() override;

	void LogStartOfLine(LogLevel logLevel) override;

	void LogEndOfLine() override;

	void LogImpl(const void * logMessage) override;
	void LogImpl(bool logMessage) override;
	void LogImpl(uint8_t logMessage) override;
	void LogImpl(uint16_t logMessage) override;
	void LogImpl(uint32_t logMessage) override;
	void LogImpl(uint64_t logMessage) override;
	void LogImpl(int8_t logMessage) override;
	void LogImpl(int16_t logMessage) override;
	void LogImpl(int32_t logMessage) override;
	void LogImpl(int64_t logMessage) override;
	void LogImpl(float logMessage) override;
	void LogImpl(double logMessage) override;
	void LogImpl(const char * logMessage) override;
};
