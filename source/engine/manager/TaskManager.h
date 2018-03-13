#pragma once
#include "interface/ITaskManager.h"
#include "interface/ILogManager.h"

extern ILogManager* g_pLogManager;

class TaskManager : public ITaskManager
{
public:
	TaskManager() {};
	~TaskManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	void addTask(std::function<void()>& task);

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void m_threadHolder();

	unsigned int m_hardwareConcurrency;
	std::vector<std::thread> m_threadPool;
	std::mutex m_mtx;
	std::condition_variable m_cv;
	std::vector<std::function<void()>*> m_taskQueue;
};

