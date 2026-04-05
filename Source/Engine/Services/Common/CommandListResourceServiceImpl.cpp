#include "../CommandListResourceService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"

using namespace Inno;

bool CommandListResourceService::Setup(IServiceConfig* systemConfig)
{
	m_Pool.Initialize(256);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "CommandListResourceService Setup finished.");
	return true;
}

bool CommandListResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "CommandListResourceService has been terminated.");
	return true;
}

CommandListComponent* CommandListResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool CommandListResourceService::Delete(CommandListComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void CommandListResourceService::ForEach(std::function<void(CommandListComponent*)> func)
{
	m_Pool.ForEach(func);
}

void CommandListResourceService::Initialize(CommandListComponent* commandList)
{
	InitializeImpl(commandList);
}
