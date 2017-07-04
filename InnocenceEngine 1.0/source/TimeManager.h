#pragma once
#include "IEventManager.h"

class TimeManager : public IEventManager
{
public:
	TimeManager();
	~TimeManager();
	
	const double getDeltaTime();
	
private:
	void init() override;
	void update() override;
	void shutdown() override;

	std::chrono::high_resolution_clock::time_point m_startTime;
	double m_deltaTime;

};

