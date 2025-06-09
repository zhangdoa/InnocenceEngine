#include "Thread.h"

#include "LogService.h"
#include "Timer.h"

#include "../Engine.h"

using namespace Inno;
namespace Inno
{
	template <>
	inline void LogService::LogContent(Thread::ID& values)
	{
		LogContent("[Thread ID: ", values.first, " | ", std::hash<std::thread::id>{}(values.second), "]");
	}

	template <>
	inline void LogService::LogContent(const Thread::ID& values)
	{
		LogContent(const_cast<Thread::ID&>(values));
	}
}

Thread::Thread(uint32_t ThreadIndex)
{
	m_ThreadHandle = new std::thread(&Thread::Worker, this, ThreadIndex);
	m_TaskReport.reserve(256);
}

Thread::~Thread()
{
	m_Done.store(true, std::memory_order_release);
	m_TaskList.clear();

	if (m_ThreadHandle->joinable())
	{
		m_ThreadHandle->join();
		delete m_ThreadHandle;
	}
}

Thread::State Thread::GetState() const
{
	return m_State.load(std::memory_order_relaxed);
}

const RingBuffer<TaskReport, true>& Thread::GetTaskReport()
{
	return m_TaskReport;
}

void Thread::AddTask(Handle<ITask> task)
{
	if (!task)
	{
		Log(Warning, "Attempted to add a nullptr task to thread ", m_ID);
		return;
	}

	auto start = Timer::GetCurrentTimeFromEpoch();
	const auto timeout = 5000;
	while (true)
	{
		{
			if (m_State.load(std::memory_order_acquire) == Thread::State::Released)
			{
				assert(false && "Attempted to add a task to a released thread.");
				std::this_thread::yield();
				return;
			}
		}

		{
			State expected = State::Idle;
			if (m_State.compare_exchange_strong(expected, Thread::State::Busy, std::memory_order_acq_rel))
			{
				m_TaskList.emplace_back(task);
				Log(Verbose, "Task: \"", m_TaskList.back()->GetName(), "\" added to thread ", m_ID);

				m_State.store(State::Idle, std::memory_order_release);
				std::this_thread::yield();
				return;
			}
		}

		{
			State expected = State::Waiting;
			if (m_State.compare_exchange_strong(expected, Thread::State::Busy, std::memory_order_acq_rel))
			{
				m_TaskList.emplace_back(task);
				Log(Verbose, "Task: \"", m_TaskList.back()->GetName(), "\" added to thread (frozen) ", m_ID);

				m_State.store(State::Waiting, std::memory_order_release);
				std::this_thread::yield();
				return;
			}
		}

		auto now = Timer::GetCurrentTimeFromEpoch();
		if (now - start > timeout)
		{
			Log(Warning, "Adding task: \"", task->GetName(), "\" is taking longer (", timeout, "ms) than expected.");
			start = now;
		}
		
		std::this_thread::yield();
	}

	assert(0);
}

inline bool Thread::ExecuteTask(Handle<ITask> task)
{
	if (!task)
	{
		Log(Error, "Trying to execute a nullptr task on thread ", m_ID);
		return false;
	}

	TaskReport l_TaskReport;
	l_TaskReport.m_ThreadID = m_ID.first;
	l_TaskReport.m_StartTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
	l_TaskReport.m_TaskName = task->GetName();

	bool l_result = task->TryToExecute();

	l_TaskReport.m_FinishTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
	m_TaskReport.emplace_back(l_TaskReport);

	auto timeSpent = l_TaskReport.m_FinishTime - l_TaskReport.m_StartTime;
	const int threshold = 100000;
	if (timeSpent > threshold)
	{
		Log(Warning, "Task: \"", task->GetName(), "\" took ", timeSpent, "us to execute on thread ", m_ID);
	}

	return l_result;
}

void Thread::Worker(uint32_t ThreadIndex)
{
	auto l_ID = std::this_thread::get_id();
	m_ID = std::make_pair(ThreadIndex, l_ID);
	Log(Success, "Thread ", m_ID, " has been created.");

	while (!m_Done.load(std::memory_order_acquire))
	{
		if (m_State.load(std::memory_order_acquire) == Thread::State::Waiting)
		{
			std::this_thread::yield();
			continue;
		}

		State expected = Thread::State::Idle;
		if (m_State.compare_exchange_strong(expected, Thread::State::Busy, std::memory_order_acq_rel))
		{
			if (m_TaskList.empty())
			{
				m_State.store(Thread::State::Idle, std::memory_order_release);
				std::this_thread::yield();
				continue;
			}

			for (auto& task : m_TaskList)
			{
				if (!task)
				{
					Log(Warning, "An empty task was detected in thread ", m_ID);
					continue;
				}

				bool l_result = ExecuteTask(task);
			}

			std::atomic_thread_fence(std::memory_order_acquire);

			m_TaskList.erase(
				std::remove_if(m_TaskList.begin(), m_TaskList.end(),
					[](const Handle<ITask>& task)
					{
						return task->CanBeRemoved();
					}),
				m_TaskList.end());

			m_State.store(Thread::State::Idle, std::memory_order_release);
		}

		std::this_thread::yield();
	}

	m_State.store(Thread::State::Released, std::memory_order_release);
	Log(Success, "Thread ", m_ID, " has been released.");
}

void Thread::Freeze()
{
	Log(Verbose, "Freezing thread ", m_ID, "...");

	while (true)
	{
		{
			if (m_State.load(std::memory_order_acquire) == Thread::State::Waiting)
			{
				Log(Verbose, "Thread ", m_ID, " has already been frozen.");
				return;
			}
		}

		State expected = State::Idle;
		if (m_State.compare_exchange_strong(expected, State::Waiting, std::memory_order_acq_rel))
		{
			for (auto& task : m_TaskList)
			{
				task->Deactivate();
			}

			Log(Verbose, "Thread ", m_ID, " has been frozen.");
			return;
		}

		std::this_thread::yield();
	}
}

void Thread::Unfreeze()
{
	Log(Verbose, "Unfreezing thread ", m_ID, "...");

	while (true)
	{
		{
			if (m_State.load(std::memory_order_acquire) != Thread::State::Waiting)
			{
				Log(Verbose, "Thread ", m_ID, " is already unfrozen.");
				return;
			}
		}

		State expected = State::Waiting;
		if (m_State.compare_exchange_strong(expected, State::Idle, std::memory_order_acq_rel))
		{
			for (auto& task : m_TaskList)
			{
				if (task->GetType() == ITask::Type::Recurrent)
					task->Activate();
			}

			Log(Verbose, "Thread ", m_ID, " has been unfrozen.");
			return;
		}

		std::this_thread::yield();
	}
}
