#include "TaskSystem.h"
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoTaskSystemNS
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::atomic_bool m_done = false;
	ThreadSafeQueue<std::unique_ptr<IThreadTask>> m_workQueue;
	std::vector<std::thread> m_threads;

	void worker(void)
	{
		while (!m_done)
		{
			std::unique_ptr<IThreadTask> pTask{ nullptr };
			if (m_workQueue.waitPop(pTask))
			{
				pTask->execute();
			}
		}
	}

	void destroy(void)
	{
		m_done = true;
		m_workQueue.invalidate();
		for (auto& thread : m_threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}
}

INNO_SYSTEM_EXPORT bool InnoTaskSystem::setup()
{
	auto l_numThreads = std::max<unsigned int>(std::thread::hardware_concurrency(), 2u) - 1u;
	try
	{
		for (std::uint32_t i = 0u; i < l_numThreads; ++i)
		{
			InnoTaskSystemNS::m_threads.emplace_back(&InnoTaskSystemNS::worker);
		}
	}
	catch (...)
	{
		InnoTaskSystemNS::destroy();
		throw;
	}
	InnoTaskSystemNS::m_objectStatus = objectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTaskSystem::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(logType::INNO_DEV_SUCCESS, "TaskSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTaskSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTaskSystem::terminate()
{
	InnoTaskSystemNS::m_objectStatus = objectStatus::STANDBY;
	InnoTaskSystemNS::destroy();
	InnoTaskSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(logType::INNO_DEV_SUCCESS, "TaskSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT objectStatus InnoTaskSystem::getStatus()
{
	return 	InnoTaskSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT void InnoTaskSystem::addTask(std::unique_ptr<IThreadTask>&& task)
{
	InnoTaskSystemNS::m_workQueue.push(std::move(task));
}
