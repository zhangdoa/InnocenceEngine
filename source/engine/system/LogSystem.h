#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

namespace InnoLogSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	void printLog(double logMessage);
	void printLog(const std::string& logMessage);
	void printLog(const vec2& logMessage);
	void printLog(const vec4& logMessage);
	void printLog(const mat4& logMessage);

	objectStatus getStatus();
};
