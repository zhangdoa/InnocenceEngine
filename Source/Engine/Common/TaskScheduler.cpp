#include "TaskScheduler.h"

#include "LogService.h"
#include "Timer.h"
#include "Thread.h"

#include "../Engine.h"

using namespace Inno;
;

TaskScheduler::TaskScheduler()
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
		delete this;
		throw;
	}
}

TaskScheduler::~TaskScheduler()
{
	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i].reset();
	}
}

void TaskScheduler::Freeze()
{
	Log(Verbose, "Freezing all thread workers...");

	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i]->Freeze();
	}

	Log(Verbose, "All thread workers have been frozen.");
}

void TaskScheduler::Unfreeze()
{
	Log(Verbose, "Unfreezing thread workers...");

	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i]->Unfreeze();
	}

	Log(Verbose, "All thread workers have been unfrozen.");
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