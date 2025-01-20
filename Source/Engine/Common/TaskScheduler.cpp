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
    Reset();
}

void Inno::TaskScheduler::Reset()
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

uint32_t TaskScheduler::GenerateThreadIndex(uint32_t threadIndex)
{
	if (threadIndex != std::numeric_limits<uint32_t>::max())
	{
		return threadIndex;
	}
	else
	{
		std::random_device RD;
		std::mt19937 Gen(RD());
		std::uniform_int_distribution<> Dis(0, (uint32_t)m_NumThreads - 1);
		return Dis(Gen);
	}
}

void TaskScheduler::AddTask(Handle<ITask> task, uint32_t threadIndex)
{
	m_Threads[threadIndex]->AddTask(task);
}

size_t TaskScheduler::GetThreadCounts()
{
	return m_NumThreads;
}

const RingBuffer<TaskReport, true>& TaskScheduler::GetTaskReport(uint32_t threadIndex)
{
	return m_Threads[threadIndex]->GetTaskReport();
}
