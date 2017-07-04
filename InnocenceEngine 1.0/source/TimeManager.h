#pragma once
#include "IEventManager.h"

class TimeManager : public IEventManager
{
public:
	TimeManager();
	~TimeManager();

	typedef std::chrono::high_resolution_clock clock;

	const double getDeltaTime();
	
private:
	void init() override;
	void update() override;
	void shutdown() override;

	clock::time_point m_startTime;
	double m_deltaTime;

};

