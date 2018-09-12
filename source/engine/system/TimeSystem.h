#pragma once
#include <sstream>
#include <chrono>
#include <ctime>
#include "../common/InnoType.h"

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

	objectStatus m_TimeSystemStatus = objectStatus::SHUTDOWN;

	const double m_frameTime = (1.0 / 120.0) * 1000.0 * 1000.0;
	time_t m_gameStartTime;
	std::chrono::high_resolution_clock::time_point m_updateStartTime;
	long long m_deltaTime;
	double m_unprocessedTime;
};
