#pragma once
#include <sstream>
#include <chrono>
#include <ctime>
#include "../../common/InnoType.h"

namespace InnoTimeSystem
{
	__declspec(dllexport) void setup();
	__declspec(dllexport) void initialize();
	__declspec(dllexport) void update();
	__declspec(dllexport) void shutdown();

	__declspec(dllexport) const time_t getGameStartTime();
	__declspec(dllexport) const long long getDeltaTime();
	__declspec(dllexport) const long long getcurrentTime();
	__declspec(dllexport) const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z);
	__declspec(dllexport) const std::string getCurrentTimeInLocal(std::chrono::hours timezone_adjustment = std::chrono::hours(8));
	__declspec(dllexport) const std::string getCurrentTimeInLocalForOutput(std::chrono::hours timezone_adjustment = std::chrono::hours(8));

	__declspec(dllexport) objectStatus getStatus();
};
