#pragma once
#include "../interface/IManager.h"
#include "LogManager.h"

class TaskManager : public IManager
{
public:
	TaskManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static TaskManager& getInstance()
	{
		static TaskManager instance;
		return instance;
	}

private:
	~TaskManager() {};

	void m_threadHolder();

	unsigned int m_hardwareConcurrency;
	std::vector<std::thread> m_threadPool;
	std::condition_variable m_cv;
	std::vector<std::function<void()>*> m_taskQueue;
};

