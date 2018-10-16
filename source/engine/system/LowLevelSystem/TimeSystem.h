#pragma once
#include <sstream>
#include <chrono>
#include <ctime>
#include "../../common/InnoType.h"

namespace InnoTimeSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	const time_t getGameStartTime();
	const long long getDeltaTime();
	const long long getcurrentTime();
	const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z);
	const std::string getCurrentTimeInLocal(std::chrono::hours timezone_adjustment = std::chrono::hours(8));
	const std::string getCurrentTimeInLocalForOutput(std::chrono::hours timezone_adjustment = std::chrono::hours(8));

	objectStatus getStatus();
};
