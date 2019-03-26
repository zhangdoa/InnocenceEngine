#pragma once
#include "../ILogSystem.h"

class InnoLogSystem : INNO_IMPLEMENT ILogSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoLogSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	void printLog(double logMessage) override;
	void printLog(const vec2& logMessage) override;
	void printLog(const vec4& logMessage) override;
	void printLog(const mat4& logMessage) override;
	void printLog(LogType LogType, const std::string& logMessage) override;

	ObjectStatus getStatus() override;
};
