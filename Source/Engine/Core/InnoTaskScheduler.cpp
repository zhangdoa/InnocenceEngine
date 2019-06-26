#include "InnoTaskScheduler.h"
#include "../Common/InnoContainer.h"
#include "InnoLogger.h"
#include "InnoTimer.h"

enum class ThreadStatus { Idle, Busy };

class InnoThread
{
public:
	explicit InnoThread(unsigned int ThreadIndex)
	{
		m_ThreadHandle = new std::thread(&InnoThread::Worker, this, ThreadIndex);
	};

	~InnoThread(void)
	{
		m_Done = true;
		m_WorkQueue.invalidate();
		if (m_ThreadHandle->joinable())
		{
			m_ThreadHandle->join();
			delete m_ThreadHandle;
		}
	};

	InnoThread(const InnoThread& rhs) = delete;
	InnoThread& operator=(const InnoThread& rhs) = delete;
	InnoThread(InnoThread&& other) = default;
	InnoThread& operator=(InnoThread&& other) = default;

	ThreadStatus GetStatus() const
	{
		return m_ThreadStatus;
	}

	IInnoTask* AddTask(std::unique_ptr<IInnoTask>&& task)
	{
		m_WorkQueue.push(std::move(task));
		return task.get();
	}

private:
	std::string GetThreadID()
	{
		std::stringstream ss;
		ss << m_ID.second;
		return ss.str();
	}

	void Worker(unsigned int ThreadIndex)
	{
		auto l_ID = std::this_thread::get_id();
		m_ID = std::make_pair(ThreadIndex, l_ID);
		m_ThreadStatus = ThreadStatus::Idle;
		InnoLogger::Log(LogLevel::Success, "InnoTaskScheduler: Thread ", GetThreadID().c_str(), " has been occupied.");

		while (!m_Done)
		{
			std::unique_ptr<IInnoTask> pTask{ nullptr };
			if (m_WorkQueue.waitPop(pTask))
			{
				m_ThreadStatus = ThreadStatus::Busy;
#if defined _DEBUG
				auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
#endif
				pTask->Execute();
#if defined _DEBUG
				auto l_FinishTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
				auto l_Duration = l_FinishTime - l_StartTime;
				InnoTaskReport l_TaskReport = { (float)l_Duration, m_ID.first, pTask->GetName() };
#endif
				m_ThreadStatus = ThreadStatus::Idle;
			}
		}

		m_ThreadStatus = ThreadStatus::Idle;
		InnoLogger::Log(LogLevel::Success, "InnoTaskScheduler: Thread ", GetThreadID().c_str(), " has been released.");
	}

	std::thread* m_ThreadHandle;
	std::pair<unsigned int, std::thread::id> m_ID;
	std::atomic<ThreadStatus> m_ThreadStatus;
	std::atomic_bool m_Done = false;
	ThreadSafeQueue<std::unique_ptr<IInnoTask>> m_WorkQueue;
};

namespace InnoTaskSchedulerNS
{
	std::atomic_uint m_NumThreads = 0;
	std::vector<std::unique_ptr<InnoThread>> m_Threads;
	std::atomic_bool m_isAllThreadsIdle = true;
}

bool InnoTaskScheduler::Setup()
{
	InnoTaskSchedulerNS::m_NumThreads = std::max<unsigned int>(std::thread::hardware_concurrency(), 2u);

	InnoTaskSchedulerNS::m_Threads.resize(InnoTaskSchedulerNS::m_NumThreads);

	try
	{
		for (std::uint32_t i = 0u; i < InnoTaskSchedulerNS::m_NumThreads; ++i)
		{
			InnoTaskSchedulerNS::m_Threads[i] = std::make_unique<InnoThread>(i);
		}
	}
	catch (...)
	{
		Terminate();
		throw;
	}

	return true;
}

bool InnoTaskScheduler::Initialize()
{
	return true;
}

bool InnoTaskScheduler::Update()
{
	return true;
}

bool InnoTaskScheduler::Terminate()
{
	for (size_t i = 0; i < InnoTaskSchedulerNS::m_Threads.size(); i++)
	{
		InnoTaskSchedulerNS::m_Threads[i].reset();
	}
	return true;
}

void InnoTaskScheduler::WaitSync()
{
	while (!InnoTaskSchedulerNS::m_isAllThreadsIdle)
	{
		for (size_t i = 0; i < InnoTaskSchedulerNS::m_Threads.size(); i++)
		{
			InnoTaskSchedulerNS::m_isAllThreadsIdle = (InnoTaskSchedulerNS::m_Threads[i]->GetStatus() == ThreadStatus::Idle);
		}
	}
	InnoLogger::Log(LogLevel::Verbose, "InnoTaskScheduler: Reached synchronization point");
}

IInnoTask * InnoTaskScheduler::AddTaskImpl(std::unique_ptr<IInnoTask>&& task, int threadID)
{
	int l_ThreadIndex;
	if (threadID != -1)
	{
		l_ThreadIndex = threadID;
	}
	else
	{
		std::random_device RD;
		std::mt19937 Gen(RD());
		std::uniform_int_distribution<> Dis(0, InnoTaskSchedulerNS::m_NumThreads - 1);
		l_ThreadIndex = Dis(Gen);
	}

	InnoTaskSchedulerNS::m_Threads[l_ThreadIndex]->AddTask(std::move(task));
	return task.get();
}