#include "../ShaderProgramResourceService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;

bool ShaderProgramResourceService::Setup(IServiceConfig* systemConfig)
{
	m_Pool.Initialize(256);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, " Setup finished.");
	return true;
}

bool ShaderProgramResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, " terminated.");
	return true;
}

ShaderProgramComponent* ShaderProgramResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool ShaderProgramResourceService::Delete(ShaderProgramComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void ShaderProgramResourceService::Initialize(ShaderProgramComponent* shaderProgram)
{
	InitializeImpl(shaderProgram);
}
