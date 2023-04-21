#include "MemorySystem.h"
#include "../Core/Memory.h"
#include "../Core/Logger.h"

using namespace Inno;
namespace Inno
{
	namespace MemorySystemNS
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	}
}

bool MemorySystem::Setup(ISystemConfig* systemConfig)
{
	MemorySystemNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool MemorySystem::Initialize()
{
	if (MemorySystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		MemorySystemNS::m_ObjectStatus = ObjectStatus::Activated;
		Logger::Log(LogLevel::Success, "MemorySystem has been initialized.");
		return true;
	}
	else
	{
		Logger::Log(LogLevel::Error, "MemorySystem: Object is not created!");
		return false;
	}
}

bool MemorySystem::Update()
{
	if (MemorySystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		MemorySystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool MemorySystem::Terminate()
{
	MemorySystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	Logger::Log(LogLevel::Success, "MemorySystem has been terminated.");
	return true;
}
ObjectStatus MemorySystem::GetStatus()
{
	return MemorySystemNS::m_ObjectStatus;
}

void* MemorySystem::allocate(size_t size)
{
	return Memory::Allocate(size);
}

bool MemorySystem::deallocate(void* ptr)
{
	Memory::Deallocate(ptr);
	return true;
}

void* MemorySystem::reallocate(void* ptr, size_t size)
{
	return Memory::Reallocate(ptr, size);
}