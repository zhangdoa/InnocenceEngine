#pragma once
#include "interface/ITaskSystem.h"
#include "interface/ILogSystem.h"

extern ILogSystem* g_pLogSystem;

class TaskSystem : public ITaskSystem
{
public:
	TaskSystem() {};
	~TaskSystem() {};

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

