#pragma once
#include "interface/ILogManager.h"
#include "interface/ITimeManager.h"

extern ITimeManager* g_pTimeManager;

class LogManager : public ILogManager
{
public:
	LogManager() {};
	~LogManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void printLog(double logMessage) const override;
	void printLog(std::string logMessage) const override;
	void printLog(const vec2& logMessage) const override;
	void printLog(const vec3& logMessage) const override;
	void printLog(const quat& logMessage) const override;
	void printLog(const mat4& logMessage) const override;
	void printLog(const std::thread::id logMessage) const override;

	const objectStatus& getStatus() const override;

protected:
	void setStatus(objectStatus objectStatus) override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};
