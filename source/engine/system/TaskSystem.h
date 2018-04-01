#pragma once
#include <condition_variable>
#include <thread>
#include <future>
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

	void addTask(void* task) override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::mutex m_mtx;
	std::condition_variable m_cv;
	std::vector<void*> m_taskQueue;
};

