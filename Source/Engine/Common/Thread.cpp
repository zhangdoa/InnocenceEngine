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
	m_TaskList.invalidate();

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

	Log(Verbose, "Task: \"", task->GetName(), "\" added to thread ", GetThreadID().c_str());
	
	m_TaskList.emplace_back(std::move(task));
	return l_result;
}

std::string Thread::GetThreadID()
{
	std::stringstream ss;
	ss << m_ID.second;
	return ss.str();
}

bool Thread::ExecuteTask(const std::shared_ptr<ITask>& task)
{
#if defined INNO_DEBUG
	auto l_StartTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
#endif

	bool l_result = task->TryToExecute();

#if defined INNO_DEBUG
	auto l_FinishTime = Timer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);
	TaskReport l_TaskReport = { l_StartTime, l_FinishTime, m_ID.first, task->GetName() };
	m_TaskReport.emplace_back(l_TaskReport);
#endif

	return l_result;
}

void Thread::Worker(uint32_t ThreadIndex)
{
	auto l_ID = std::this_thread::get_id();
	m_ID = std::make_pair(ThreadIndex, l_ID);
	Log(Success, "Thread (", ThreadIndex, ") ", GetThreadID().c_str(), " has been created.");

	while (!m_Done.load(std::memory_order_acquire))
	{
		if (m_State.load(std::memory_order_acquire) == Thread::State::Waiting)
		{
			std::this_thread::yield();
			continue;
		}

		if (m_TaskList.empty())
		{
			std::this_thread::yield();
			continue;
		}

		m_State.store(Thread::State::Busy, std::memory_order_release);

		m_TaskList.for_each([this](std::shared_ptr<ITask>& task) 
		{
			bool l_result = ExecuteTask(task);
			if (l_result && task->GetType() == ITask::Type::Once)
			{
				Log(Verbose, "Task: \"", task->GetName(), "\" is about to be removed from thread ", GetThreadID().c_str());
				task.reset();
			}
		});

		m_TaskList.erase_if([](const std::shared_ptr<ITask>& task) 
		{
			return task == nullptr;
		});

		m_State.store(Thread::State::Idle, std::memory_order_release);
	}

	m_State.store(Thread::State::Released, std::memory_order_release);
	Log(Success, "Thread ", GetThreadID().c_str(), " has been released.");
}

void Thread::Freeze()
{
	Log(Verbose, "Freezing thread ", GetThreadID().c_str(), "...");
	auto l_activeTasks = 0;

    while (true)
    {
        State expected = m_State.load(std::memory_order_acquire);
		if (expected == Thread::State::Waiting)
		{
			Log(Verbose, "Thread ", GetThreadID().c_str(), " is already frozen.");
			return;
		}

        if (m_State.compare_exchange_weak(expected, State::Waiting, std::memory_order_acq_rel))
        {
			for (auto& task : m_TaskList)
			{
				task->Deactivate();
			}

			Log(Verbose, "Thread ", GetThreadID().c_str(), " has been frozen.");
            return;
        }

		Log(Verbose, "Thread ", GetThreadID().c_str(), " is busy. Waiting for tasks to be completed...");
        std::this_thread::yield();
    }
}

void Thread::Unfreeze()
{
	Log(Verbose, "Unfreezing thread ", GetThreadID().c_str(), "...");

    while (true)
    {
        State expected = m_State.load(std::memory_order_acquire);
		if (expected == Thread::State::Idle)
		{
			Log(Verbose, "Thread ", GetThreadID().c_str(), " is already unfrozen.");
			return;
		}

        if (m_State.compare_exchange_weak(expected, State::Idle, std::memory_order_acq_rel))
        {
			m_TaskList.for_each([this](std::shared_ptr<ITask>& task) 
			{
				if (task->GetType() == ITask::Type::Recurrent)
					task->Activate();
			});

			Log(Verbose, "Thread ", GetThreadID().c_str(), " has been unfrozen.");
            return;
        }

		Log(Verbose, "Thread ", GetThreadID().c_str(), " is busy. Waiting for tasks to be completed...");
        std::this_thread::yield();
    }
}