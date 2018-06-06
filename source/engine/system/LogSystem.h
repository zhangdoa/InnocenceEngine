#pragma once
#include <sstream>
#include <iostream>
#include "interface/ILogSystem.h"
#include "interface/ITimeSystem.h"

extern ITimeSystem* g_pTimeSystem;

class LogSystem : public ILogSystem
{
public:
	LogSystem() {};
	~LogSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void printLog(double logMessage) const override;
	void printLog(const std::string& logMessage) const override;
	void printLog(const vec2& logMessage) const override;
	void printLog(const vec4& logMessage) const override;
	void printLog(const mat4& logMessage) const override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};
