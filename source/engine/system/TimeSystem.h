#pragma once
#include <sstream>
#include "interface/ITimeSystem.h"

class TimeSystem : public ITimeSystem
{
public:
	TimeSystem() {};
	~TimeSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const time_t getGameStartTime() const override;
	const double getDeltaTime() const override;
	const double getcurrentTime() const override;
	const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z) const override;
	const std::string getCurrentTimeInLocal(std::chrono::hours timezone_adjustment = std::chrono::hours(8)) const override;
	const std::string getCurrentTimeInLocalForOutput(std::chrono::hours timezone_adjustment = std::chrono::hours(8)) const override;
	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	const double m_frameTime = (1.0 / 120.0) * 1000.0 * 1000.0;
	time_t m_gameStartTime;
	std::chrono::high_resolution_clock::time_point m_updateStartTime;
	long long m_deltaTime;
	double m_unprocessedTime;
};
