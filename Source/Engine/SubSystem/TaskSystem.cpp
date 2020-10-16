#include "TaskSystem.h"

namespace InnoTaskSystemNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

ObjectStatus InnoTaskSystem::GetStatus()
{
	return InnoTaskSystemNS::m_ObjectStatus;
}

bool InnoTaskSystem::Setup(ISystemConfig* systemConfig)
{
	InnoTaskSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return InnoTaskScheduler::Setup();
}

bool InnoTaskSystem::Initialize()
{
	if (InnoTaskSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		InnoTaskSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		return InnoTaskScheduler::Initialize();
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
		return 	InnoTaskScheduler::Update();
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
	return InnoTaskScheduler::Terminate();
}

void InnoTaskSystem::waitAllTasksToFinish()
{
	InnoTaskScheduler::WaitSync();
}

const RingBuffer<InnoTaskReport, true>& InnoTaskSystem::GetTaskReport(int32_t threadID)
{
	return InnoTaskScheduler::GetTaskReport(threadID);
}

size_t InnoTaskSystem::GetTotalThreadsNumber()
{
	return InnoTaskScheduler::GetTotalThreadsNumber();
}

std::shared_ptr<IInnoTask> InnoTaskSystem::addTaskImpl(std::unique_ptr<IInnoTask>&& task, int32_t threadID)
{
	return InnoTaskScheduler::AddTaskImpl(std::move(task), threadID);
}