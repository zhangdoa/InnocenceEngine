#include "TaskSystem.h"

namespace InnoTaskSystemNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

ObjectStatus InnoTaskSystem::getStatus()
{
	return InnoTaskSystemNS::m_ObjectStatus;
}

bool InnoTaskSystem::setup()
{
	InnoTaskSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return InnoTaskScheduler::Setup();
}

bool InnoTaskSystem::initialize()
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

bool InnoTaskSystem::update()
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

bool InnoTaskSystem::terminate()
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