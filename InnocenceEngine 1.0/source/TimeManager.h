#pragma once
#include "IEventManager.h"
#include "LogManager.h"


class TimeManager : public IEventManager
{
public:
	TimeManager();
	~TimeManager();
	
	const double getGameStartTime();
	const double getDeltaTime();
	
private:
	void init() override;
	void update() override;
	void shutdown() override;

	const double m_frameTime = (1.0 / 60.0) * 1000.0;
	double m_gameStartTime;
	std::chrono::high_resolution_clock::time_point m_updateStartTime;
	double m_deltaTime;
	double m_unprocessedTime;

};

