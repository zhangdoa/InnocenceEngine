#include "TaskSystem.h"

namespace InnoTaskSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

ObjectStatus InnoTaskSystem::getStatus()
{
	return InnoTaskSystemNS::m_objectStatus;
}

bool InnoTaskSystem::setup()
{
	InnoTaskSystemNS::m_objectStatus = ObjectStatus::Created;
	return InnoTaskScheduler::Setup();
}

bool InnoTaskSystem::initialize()
{
	if (InnoTaskSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoTaskSystemNS::m_objectStatus = ObjectStatus::Activated;
		return InnoTaskScheduler::Initialize();
	}
	else
	{
		return false;
	}
}

bool InnoTaskSystem::update()
{
	if (InnoTaskSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		return 	InnoTaskScheduler::Update();
	}
	else
	{
		InnoTaskSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoTaskSystem::terminate()
{
	InnoTaskSystemNS::m_objectStatus = ObjectStatus::Terminated;
	return InnoTaskScheduler::Terminate();
}

void InnoTaskSystem::waitAllTasksToFinish()
{
	InnoTaskScheduler::WaitSync();
}

const RingBuffer<InnoTaskReport, true>& InnoTaskSystem::GetTaskReport(int threadID)
{
	return InnoTaskScheduler::GetTaskReport(threadID);
}

size_t InnoTaskSystem::GetTotalThreadsNumber()
{
	return InnoTaskScheduler::GetTotalThreadsNumber();
}

IInnoTask * InnoTaskSystem::addTaskImpl(std::unique_ptr<IInnoTask>&& task, int threadID)
{
	return InnoTaskScheduler::AddTaskImpl(std::move(task), threadID);
}