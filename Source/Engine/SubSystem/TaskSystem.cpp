#include "TaskSystem.h"

using namespace Inno;
namespace Inno
{
	namespace InnoTaskSystemNS
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	}
}

ObjectStatus InnoTaskSystem::GetStatus()
{
	return InnoTaskSystemNS::m_ObjectStatus;
}

bool InnoTaskSystem::Setup(ISystemConfig* systemConfig)
{
	InnoTaskSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return TaskScheduler::Setup();
}

bool InnoTaskSystem::Initialize()
{
	if (InnoTaskSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		InnoTaskSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		return TaskScheduler::Initialize();
	}
	else
	{
		return false;
	}
}

bool InnoTaskSystem::Update()
{
	if (InnoTaskSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return 	TaskScheduler::Update();
	}
	else
	{
		InnoTaskSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoTaskSystem::Terminate()
{
	InnoTaskSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	return TaskScheduler::Terminate();
}

void InnoTaskSystem::WaitSync()
{
	TaskScheduler::WaitSync();
}

const RingBuffer<TaskReport, true>& InnoTaskSystem::GetTaskReport(int32_t threadID)
{
	return TaskScheduler::GetTaskReport(threadID);
}

size_t InnoTaskSystem::GetThreadCounts()
{
	return TaskScheduler::GetThreadCounts();
}

std::shared_ptr<ITask> InnoTaskSystem::AddTask(std::unique_ptr<ITask>&& task, int32_t threadID)
{
	return TaskScheduler::AddTask(std::move(task), threadID);
}