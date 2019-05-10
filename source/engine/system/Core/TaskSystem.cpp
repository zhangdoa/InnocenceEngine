#include "TaskSystem.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

enum class WorkerStatus { IDLE, BUSY };

class InnoThread
{
public:
	InnoThread()
	{
		m_thread = new std::thread(&InnoThread::worker, this);
	};

	~InnoThread(void)
	{
		m_done = true;
		m_workQueue.invalidate();
		if (m_thread->joinable())
		{
			m_thread->join();
			delete m_thread;
		}
	};

	InnoThread(const InnoThread& rhs) = delete;
	InnoThread& operator=(const InnoThread& rhs) = delete;
	InnoThread(InnoThread&& other) = default;
	InnoThread& operator=(InnoThread&& other) = default;

	WorkerStatus getStatus() const
	{
		return m_threadStatus;
	}

	void* addTask(std::unique_ptr<IThreadTask>&& task)
	{
		m_workQueue.push(std::move(task));
		return task.get();
	}

private:
	std::string getThreadId()
	{
		std::stringstream ss;
		ss << m_id;
		return ss.str();
	}

	void worker(void)
	{
		m_id = std::this_thread::get_id();
		m_threadStatus = WorkerStatus::IDLE;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem: Thread " + getThreadId() + " has been occupied.");

		while (!m_done)
		{
			std::unique_ptr<IThreadTask> pTask{ nullptr };
			if (m_workQueue.waitPop(pTask))
			{
				m_threadStatus = WorkerStatus::BUSY;
				pTask->execute();
				m_threadStatus = WorkerStatus::IDLE;
			}
		}

		m_threadStatus = WorkerStatus::IDLE;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem: Thread " + getThreadId() + " has been released.");
	}

	std::thread* m_thread;
	std::thread::id m_id;
	std::atomic<WorkerStatus> m_threadStatus;
	std::atomic_bool m_done = false;
	ThreadSafeQueue<std::unique_ptr<IThreadTask>> m_workQueue;
};

INNO_PRIVATE_SCOPE InnoTaskSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	std::atomic_bool m_isAllTasksFinished = true;

	unsigned int m_numThreads = 0;
	std::vector<InnoThread*> m_threads;

	std::string getThreadId()
	{
		std::stringstream ss;
		ss << std::this_thread::get_id();
		return ss.str();
	}

	void destroy(void)
	{
		for (auto thread : m_threads)
		{
			delete thread;
		}
	}
}

bool InnoTaskSystemNS::setup()
{
	m_numThreads = std::max<unsigned int>(std::thread::hardware_concurrency(), 2u);

	m_threads.reserve(m_numThreads);

	try
	{
		for (std::uint32_t i = 0u; i < m_numThreads; ++i)
		{
			m_threads.emplace_back(new InnoThread());
		}
	}
	catch (...)
	{
		destroy();
		throw;
	}

	m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

bool InnoTaskSystemNS::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem has been initialized.");
	return true;
}

bool InnoTaskSystemNS::update()
{
	return true;
}

bool InnoTaskSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::STANDBY;
	destroy();
	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem has been terminated.");
	return true;
}

bool InnoTaskSystem::setup()
{
	return InnoTaskSystemNS::setup();
}

bool InnoTaskSystem::initialize()
{
	return InnoTaskSystemNS::initialize();
}

bool InnoTaskSystem::update()
{
	return InnoTaskSystemNS::update();
}

bool InnoTaskSystem::terminate()
{
	return InnoTaskSystemNS::terminate();
}

ObjectStatus InnoTaskSystem::getStatus()
{
	return 	InnoTaskSystemNS::m_objectStatus;
}

void* InnoTaskSystem::addTask(std::unique_ptr<IThreadTask>&& task)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, InnoTaskSystemNS::m_numThreads - 1);
	auto l_threadIndex = dis(gen);
	auto l_result = InnoTaskSystemNS::m_threads[l_threadIndex]->addTask(std::move(task));
	return task.get();
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
		for (auto thread : InnoTaskSystemNS::m_threads)
		{
			InnoTaskSystemNS::m_isAllTasksFinished = (thread->getStatus() == WorkerStatus::IDLE);
		}
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem: All threads are idle now.");
}

std::string InnoTaskSystem::getThreadId()
{
	return InnoTaskSystemNS::getThreadId();
}