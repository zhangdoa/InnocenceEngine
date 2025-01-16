#include "Thread.h"

#include "LogService.h"
#include "Timer.h"

#include "../Engine.h"

using namespace Inno;

Thread::Thread(uint32_t ThreadIndex)
{
	m_ThreadHandle = new std::thread(&Thread::Worker, this, ThreadIndex);
	m_TaskReport.reserve(256);
};

Thread::~Thread()
{
	m_Done.store(true, std::memory_order_release);
	m_WorkQueue.invalidate();

	if (m_ThreadHandle->joinable())
	{
		m_ThreadHandle->join();
		delete m_ThreadHandle;
	}
};

Thread::State Thread::GetState() const
{
	return m_State.load(std::memory_order_relaxed);
}

const RingBuffer<TaskReport, true>& Thread::GetTaskReport()
{
	return m_TaskReport;
}

std::shared_ptr<ITask> Thread::AddTask(std::shared_ptr<ITask>&& task)
{
	std::shared_ptr<ITask> l_result{ task };
	m_WorkQueue.push(std::move(task));
	return l_result;
}

std::string Thread::GetThreadID()
{
	std::stringstream ss;
	ss << m_ID.second;
	return ss.str();
}

void Thread::ExecuteTask(std::shared_ptr<ITask>&& task)
{
#if defined INNO_DEBUG
	auto l_StartTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
#endif

	bool result = task->TryToExecute();

#if defined INNO_DEBUG
	auto l_FinishTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
	TaskReport l_TaskReport = { l_StartTime, l_FinishTime, m_ID.first, task->GetName() };
	m_TaskReport.emplace_back(l_TaskReport);
#endif

	if (!result)
		AddTask(std::move(task));
}

void Thread::Worker(uint32_t ThreadIndex)
{
	auto l_ID = std::this_thread::get_id();
	m_ID = std::make_pair(ThreadIndex, l_ID);
	Log(Success, "Thread ", GetThreadID().c_str(), " has been occupied.");

	while (!m_Done.load(std::memory_order_acquire))
	{
		if (m_State.load(std::memory_order_acquire) == Thread::State::Idle)
			continue;

		std::shared_ptr<ITask> pTask{ nullptr };
		if (m_WorkQueue.waitPop(pTask))
		{
			m_State.store(Thread::State::Busy, std::memory_order_release);
			ExecuteTask(std::move(pTask));
			m_State.store(Thread::State::Idle, std::memory_order_release);
		}
	}

	m_State.store(Thread::State::Released, std::memory_order_release);
	Log(Success, "Thread ", GetThreadID().c_str(), " has been released.");
}
