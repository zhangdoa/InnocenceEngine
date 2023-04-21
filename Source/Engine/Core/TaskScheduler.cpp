#include "TaskScheduler.h"
#include "../Common/Container.h"
#include "Logger.h"
#include "Timer.h"

using namespace Inno;
namespace Inno
{
	enum class ThreadState { Idle, Busy };

	class Thread
	{
	public:
		explicit Thread(uint32_t ThreadIndex)
		{
			m_ThreadHandle = new std::thread(&Thread::Worker, this, ThreadIndex);
			m_TaskReport.reserve(256);
		};

		~Thread(void)
		{
			m_Done = true;
			m_WorkQueue.invalidate();
			if (m_ThreadHandle->joinable())
			{
				m_ThreadHandle->join();
				delete m_ThreadHandle;
			}
		};

		Thread(const Thread& rhs) = delete;
		Thread& operator=(const Thread& rhs) = delete;

		ThreadState GetState() const;
		size_t GetUnfinishedWorkCount();
		const RingBuffer<TaskReport, true>& GetTaskReport();

		std::shared_ptr<ITask> AddTask(std::shared_ptr<ITask>&& task);

	private:
		std::string GetThreadID();

		void Worker(uint32_t ThreadIndex);

		void ExecuteTask(std::shared_ptr<ITask>&& task);

		std::thread* m_ThreadHandle;
		std::pair<uint32_t, std::thread::id> m_ID;
		std::atomic<ThreadState> m_ThreadState;
		std::atomic_bool m_Done = false;
		ThreadSafeQueue<std::shared_ptr<ITask>> m_WorkQueue;
		RingBuffer<TaskReport, true> m_TaskReport;
	};

	namespace TaskSchedulerNS
	{
		std::atomic_size_t m_NumThreads = 0;
		std::vector<std::unique_ptr<Thread>> m_Threads;
	}
}

using namespace TaskSchedulerNS;

bool TaskScheduler::Setup()
{
	m_NumThreads = std::max<size_t>(std::thread::hardware_concurrency(), 2u);

	m_Threads.resize(m_NumThreads);

	try
	{
		for (std::uint32_t i = 0u; i < m_NumThreads; ++i)
		{
			m_Threads[i] = std::make_unique<Thread>(i);
		}
	}
	catch (...)
	{
		Terminate();
		throw;
	}

	return true;
}

bool TaskScheduler::Initialize()
{
	return true;
}

bool TaskScheduler::Update()
{
	return true;
}

bool TaskScheduler::Terminate()
{
	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i].reset();
	}
	return true;
}

void TaskScheduler::WaitSync()
{
	std::atomic_bool l_isAllThreadsIdle = false;

	while (!l_isAllThreadsIdle)
	{
		for (size_t i = 0; i < m_Threads.size(); i++)
		{
			l_isAllThreadsIdle = (m_Threads[i]->GetUnfinishedWorkCount() == 0);
		}
	}

	Logger::Log(LogLevel::Verbose, "TaskScheduler: Reached synchronization point");
}

std::shared_ptr<ITask> TaskScheduler::AddTask(std::unique_ptr<ITask>&& task, int32_t threadID)
{
	int32_t l_ThreadIndex;
	if (threadID != -1)
	{
		l_ThreadIndex = threadID;
	}
	else
	{
		std::random_device RD;
		std::mt19937 Gen(RD());
		std::uniform_int_distribution<> Dis(0, (uint32_t)m_NumThreads - 1);
		l_ThreadIndex = Dis(Gen);
	}

	return m_Threads[l_ThreadIndex]->AddTask(std::move(task));
}

size_t TaskScheduler::GetThreadCounts()
{
	return m_NumThreads;
}

const RingBuffer<TaskReport, true>& TaskScheduler::GetTaskReport(int32_t threadID)
{
	return m_Threads[threadID]->GetTaskReport();
}

inline ThreadState Thread::GetState() const
{
	return m_ThreadState;
}

size_t Thread::GetUnfinishedWorkCount()
{
	return m_WorkQueue.size();
}

inline const RingBuffer<TaskReport, true>& Thread::GetTaskReport()
{
	return m_TaskReport;
}

inline std::shared_ptr<ITask> Thread::AddTask(std::shared_ptr<ITask>&& task)
{
	std::shared_ptr<ITask> l_result{ task };
	m_WorkQueue.push(std::move(task));
	return l_result;
}

inline std::string Thread::GetThreadID()
{
	std::stringstream ss;
	ss << m_ID.second;
	return ss.str();
}

inline void Thread::ExecuteTask(std::shared_ptr<ITask>&& task)
{
#if defined INNO_DEBUG
	auto l_StartTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
#endif

	task->Execute();

#if defined INNO_DEBUG
	auto l_FinishTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
	TaskReport l_TaskReport = { l_StartTime, l_FinishTime, m_ID.first, task->GetName() };
	m_TaskReport.emplace_back(l_TaskReport);
#endif
}

inline void Thread::Worker(uint32_t ThreadIndex)
{
	auto l_ID = std::this_thread::get_id();
	m_ID = std::make_pair(ThreadIndex, l_ID);
	m_ThreadState = ThreadState::Idle;
	Logger::Log(LogLevel::Success, "TaskScheduler: Thread ", GetThreadID().c_str(), " has been occupied.");

	while (!m_Done)
	{
		std::shared_ptr<ITask> pTask{ nullptr };
		if (m_WorkQueue.waitPop(pTask))
		{
			m_ThreadState = ThreadState::Busy;
			auto l_upstreamTask = pTask->GetUpstreamTask();

			if (l_upstreamTask != nullptr)
			{
				if (l_upstreamTask->IsFinished())
				{
					ExecuteTask(std::move(pTask));
				}
				else
				{
					AddTask(std::move(pTask));
				}
			}
			else
			{
				ExecuteTask(std::move(pTask));
			}

			m_ThreadState = ThreadState::Idle;
		}
	}

	m_ThreadState = ThreadState::Idle;
	Logger::Log(LogLevel::Success, "TaskScheduler: Thread ", GetThreadID().c_str(), " has been released.");
}
