#pragma once
#include "BaseManager.h"
#include "LogManager.h"

extern LogManager* g_pLogManager;

class TaskManager : public BaseManager
{
public:
	TaskManager() {};
	~TaskManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void addTask(std::function<void()>& task);

private:
	void m_threadHolder();

	unsigned int m_hardwareConcurrency;
	std::vector<std::thread> m_threadPool;
	std::mutex m_mtx;
	std::condition_variable m_cv;
	std::vector<std::function<void()>*> m_taskQueue;
};

