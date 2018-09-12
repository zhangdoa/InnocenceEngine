#pragma once
#include "../common/InnoType.h"
#include "../common/InnoConcurrency.h"

namespace InnoTaskSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	template <typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)
	{
		return m_threadPool->submit(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	objectStatus m_TaskSystemStatus = objectStatus::SHUTDOWN;

	InnoThreadPool* m_threadPool;
};

