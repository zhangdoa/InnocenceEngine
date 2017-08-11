#pragma once
#include "../manager/IEventManager.h"
#include "../manager/LogManager.h"


class TimeManager : public IEventManager
{
public:
	~TimeManager();

	static TimeManager& getInstance()
	{
		static TimeManager instance;
		return instance;
	}

	const __time64_t getGameStartTime() const;
	const double getDeltaTime() const;
	static std::string getCurrentTimeInLocal();

private:
	TimeManager();

	const double m_frameTime = (1.0 / 60.0) * 1000.0;
	__time64_t m_gameStartTime;
	std::chrono::high_resolution_clock::time_point m_updateStartTime;
	double m_deltaTime;
	double m_unprocessedTime;

	void init() override;
	void update() override;
	void shutdown() override;
};

