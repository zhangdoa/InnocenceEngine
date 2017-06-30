#pragma once
#include "IEventManager.h"
class TimeManager : public IEventManager
{
public:
	TimeManager();
	~TimeManager();

	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::duration<float, std::milli> duration;

	void exec(eventMessage eventMessage) override;
	
private:

	void init();
	void update();
	void shutdown();
	void reportError() override;

	double _delta;
};

