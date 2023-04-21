#include "TaskSystem.h"

using namespace Inno;
namespace Inno
{
	namespace TaskSystemNS
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	}
}

ObjectStatus TaskSystem::GetStatus()
{
	return TaskSystemNS::m_ObjectStatus;
}

bool TaskSystem::Setup(ISystemConfig* systemConfig)
{
	TaskSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return TaskScheduler::Setup();
}

bool TaskSystem::Initialize()
{
	if (TaskSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		TaskSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		return TaskScheduler::Initialize();
	}
	else
	{
		return false;
	}
}

bool TaskSystem::Update()
{
	if (TaskSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return 	TaskScheduler::Update();
	}
	else
	{
		TaskSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool TaskSystem::Terminate()
{
	TaskSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	return TaskScheduler::Terminate();
}

void TaskSystem::WaitSync()
{
	TaskScheduler::WaitSync();
}

const RingBuffer<TaskReport, true>& TaskSystem::GetTaskReport(int32_t threadID)
{
	return TaskScheduler::GetTaskReport(threadID);
}

size_t TaskSystem::GetThreadCounts()
{
	return TaskScheduler::GetThreadCounts();
}

std::shared_ptr<ITask> TaskSystem::AddTask(std::unique_ptr<ITask>&& task, int32_t threadID)
{
	return TaskScheduler::AddTask(std::move(task), threadID);
}