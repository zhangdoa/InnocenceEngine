#pragma once
#include "IEventManager.h"
#include "LogManager.h"


class TimeManager : public IEventManager
{
public:
	TimeManager();
	~TimeManager();

	static TimeManager& getInstance()
	{
		static TimeManager instance;
		return instance;
	}

	const __time64_t getGameStartTime();
	const double getDeltaTime();
	static std::string getCurrentTimeInLocal();
private:
	void init() override;
	void update() override;
	void shutdown() override;

	const double m_frameTime = (1.0 / 60.0) * 1000.0;
	__time64_t m_gameStartTime;
	std::chrono::high_resolution_clock::time_point m_updateStartTime;
	double m_deltaTime;
	double m_unprocessedTime;

};

