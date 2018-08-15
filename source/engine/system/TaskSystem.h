#pragma once
#include "interface/ITaskSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
#include "component/InnoConcurrency.h"

extern IMemorySystem* g_pMemorySystem;
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

	template <typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)
	{
		return m_threadPool->submit(std::forward<Func>(func), std::forward<Args>(args)...);
	}

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	InnoThreadPool* m_threadPool;
};

