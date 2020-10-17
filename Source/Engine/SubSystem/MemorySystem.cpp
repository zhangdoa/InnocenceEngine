#include "MemorySystem.h"
#include "../Core/InnoMemory.h"
#include "../Core/InnoLogger.h"

using namespace Inno;
namespace Inno
{
	namespace InnoMemorySystemNS
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	}
}

bool InnoMemorySystem::Setup(ISystemConfig* systemConfig)
{
	InnoMemorySystemNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoMemorySystem::Initialize()
{
	if (InnoMemorySystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		InnoMemorySystemNS::m_ObjectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "MemorySystem has been initialized.");
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "MemorySystem: Object is not created!");
		return false;
	}
}

bool InnoMemorySystem::Update()
{
	if (InnoMemorySystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		InnoMemorySystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoMemorySystem::Terminate()
{
	InnoMemorySystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "MemorySystem has been terminated.");
	return true;
}
ObjectStatus InnoMemorySystem::GetStatus()
{
	return InnoMemorySystemNS::m_ObjectStatus;
}

void* InnoMemorySystem::allocate(size_t size)
{
	return InnoMemory::Allocate(size);
}

bool InnoMemorySystem::deallocate(void* ptr)
{
	InnoMemory::Deallocate(ptr);
	return true;
}

void* InnoMemorySystem::reallocate(void* ptr, size_t size)
{
	return InnoMemory::Reallocate(ptr, size);
}