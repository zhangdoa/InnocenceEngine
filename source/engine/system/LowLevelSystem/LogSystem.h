#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoMath.h"

namespace InnoLogSystem
{
	__declspec(dllexport) void setup();
	__declspec(dllexport) void initialize();
	__declspec(dllexport) void update();
	__declspec(dllexport) void shutdown();

	__declspec(dllexport) void printLog(double logMessage);
	__declspec(dllexport) void printLog(const std::string& logMessage);
	__declspec(dllexport) void printLog(const vec2& logMessage);
	__declspec(dllexport) void printLog(const vec4& logMessage);
	__declspec(dllexport) void printLog(const mat4& logMessage);

	__declspec(dllexport) objectStatus getStatus();
};
