#include "TaskSystem.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

enum class WorkerStatus { IDLE, BUSY };

INNO_PRIVATE_SCOPE InnoTaskSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	std::atomic_bool m_done = false;

	std::atomic_bool m_isAllTasksFinished = true;

	ThreadSafeQueue<std::unique_ptr<IThreadTask>> m_workQueue;
	std::vector<std::thread> m_threads;

	std::unordered_map<std::thread::id, std::atomic<WorkerStatus>> m_threadStatus;

	void worker(void)
	{
		auto l_id = std::this_thread::get_id();
		InnoTaskSystemNS::m_threadStatus.emplace(l_id, WorkerStatus::IDLE);
		auto l_it = m_threadStatus.find(l_id);

		while (!m_done)
		{
			std::unique_ptr<IThreadTask> pTask{ nullptr };
			if (m_workQueue.waitPop(pTask))
			{
				l_it->second = WorkerStatus::BUSY;
				pTask->execute();
				l_it->second = WorkerStatus::IDLE;
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

bool InnoTaskSystem::setup()
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
	InnoTaskSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

bool InnoTaskSystem::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem has been initialized.");
	return true;
}

bool InnoTaskSystem::update()
{
	return true;
}

bool InnoTaskSystem::terminate()
{
	InnoTaskSystemNS::m_objectStatus = ObjectStatus::STANDBY;
	InnoTaskSystemNS::destroy();
	InnoTaskSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem has been terminated.");
	return true;
}

ObjectStatus InnoTaskSystem::getStatus()
{
	return 	InnoTaskSystemNS::m_objectStatus;
}

void InnoTaskSystem::addTask(std::unique_ptr<IThreadTask>&& task)
{
	InnoTaskSystemNS::m_workQueue.push(std::move(task));
}

void InnoTaskSystem::shrinkFutureContainer(std::vector<InnoFuture<void>>& rhs)
{
	auto l_removeResult = std::remove_if(rhs.begin(), rhs.end(), [](InnoFuture<void>& val) {
		return val.isReady();
	});
	rhs.erase(l_removeResult, rhs.end());

	rhs.shrink_to_fit();
}

void InnoTaskSystem::waitAllTasksToFinish()
{
	while (!InnoTaskSystemNS::m_isAllTasksFinished)
	{
		for (auto& i : InnoTaskSystemNS::m_threadStatus)
		{
			InnoTaskSystemNS::m_isAllTasksFinished = (i.second == WorkerStatus::IDLE);
		}
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem: All threads are idle now.");
}

std::string InnoTaskSystem::getThreadId()
{
	std::stringstream ss;
	ss << std::this_thread::get_id();
	return ss.str();
}